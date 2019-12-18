#include <stdlib.h>
#include <ruby.h>
#include <client.h>
#include <logger.h>
#include <cJSON.h>

/* ruby public interface is in Init_ruby_snowflake_client */

// VALUE is in ruby.h as generic pointer
// m for module
// c for class
static VALUE rb_cEncapsulateSfConnect;

// the following 3 statements tell ruby how to gc the object we return
void sf_wrapper_free(void *sf) {
    snowflake_term(sf);  // is this too slow to call here?
}

size_t sf_wrapper_size(const void *sf) {
    return sizeof(SF_CONNECT);
}

static const rb_data_type_t sf_wrapper = {
        .wrap_struct_name = "SF_CONNECT",
        .function = {
                .dmark = NULL,  // TODO add all the connection args as ruby readable instance vars and then impl this method
                .dfree = sf_wrapper_free,
                .dsize = sf_wrapper_size,
        },
        .data = NULL,
        .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

// ruby calls these 2 methods to create a new instance
VALUE sf_wrapper_alloc(VALUE self) {
    SF_STATUS status = snowflake_global_init(NULL, SF_LOG_DEBUG, NULL);  // arg0 is const char *log_path
    if (status != SF_STATUS_SUCCESS) {
        rb_raise(rb_eIOError, "%d", status);
    }

    /* allocate */
    SF_CONNECT *sf = snowflake_init();

    /* wrap */
    return TypedData_Wrap_Struct(self, &sf_wrapper, sf);
}

VALUE sf_wrapper_m_initialize(VALUE self, VALUE host, VALUE account, VALUE warehouse, VALUE database, VALUE schema,
                              VALUE user, VALUE password, VALUE role, VALUE timezone, VALUE port) {
    SF_CONNECT *sf;
    TypedData_Get_Struct(self, SF_CONNECT, &sf_wrapper, sf);

    snowflake_set_attribute(sf, SF_CON_HOST, StringValueCStr(host));
    snowflake_set_attribute(sf, SF_CON_ACCOUNT, StringValueCStr(account));
    snowflake_set_attribute(sf, SF_CON_WAREHOUSE, StringValueCStr(warehouse));
    snowflake_set_attribute(sf, SF_CON_DATABASE, StringValueCStr(database));
    snowflake_set_attribute(sf, SF_CON_SCHEMA, StringValueCStr(schema));
    snowflake_set_attribute(sf, SF_CON_USER, StringValueCStr(user));
    snowflake_set_attribute(sf, SF_CON_PASSWORD, StringValueCStr(password));

    if (role != Qnil) { snowflake_set_attribute(sf, SF_CON_ROLE, StringValueCStr(role)); }

    snowflake_set_attribute(sf, SF_CON_AUTOCOMMIT, &SF_BOOLEAN_TRUE);

    if (timezone != Qnil) { snowflake_set_attribute(sf, SF_CON_TIMEZONE, StringValueCStr(timezone)); }

    if (port != Qnil) {
        snowflake_set_attribute(sf, SF_CON_PORT, StringValueCStr(port));
    }

    SF_STATUS status = snowflake_connect(sf);
    if (status != SF_STATUS_SUCCESS) {
        rb_raise(rb_eIOError, "%d: %s", sf->error.error_code, sf->error.msg);
    }

    return self;
}

static VALUE
dump_error(SF_ERROR_STRUCT *error) {
    char msg[128];
    sprintf(msg, "%d: %s", error->error_code, error->msg);
    return (rb_str_new2(msg));
}

// adaptation of snowflake_column_as_timestamp to just return universal time payload rather than parse into
// something ruby doesn't understand. May be worthwhile figuring out how to create ruby Time &/or Date instances
SF_STATUS
snowflake_column_as_universal_time(SF_STMT *sfstmt, int idx, float64 *value_ptr) {
    // Get column (pulled guts out of private `_snowflake_get_cJSON_column`)
    cJSON *column = snowflake_cJSON_GetArrayItem(sfstmt->cur_row, idx - 1);

    if (!column) return SF_STATUS_ERROR_MISSING_COLUMN_IN_ROW;
    // end snippet

    if (snowflake_cJSON_IsNull(column)) {
        *value_ptr = 0.0; // beginning of epoch but should never occur bc there's a test for null before this call
        return SF_STATUS_SUCCESS;
    }

    char *rest_of_string;
    float64 seconds = strtod(column->valuestring, &rest_of_string);

    // timezone
    if (rest_of_string < column->valuestring + strlen(column->valuestring)) {
        int64 tzoffset = strtoll(rest_of_string, NULL, 10);
        if (tzoffset != 0) {
            rb_warn("Source %s converted to %f seconds + %lld minutes", column->valuestring, seconds, tzoffset);
            seconds += tzoffset * 60.0;
        }
    }

    *value_ptr = seconds;

    return SF_STATUS_SUCCESS;
}

/**
 * modeled after https://github.com/snowflakedb/libsnowflakeclient/blob/master/tests/test_crud.c#_fetch_data
 * Executes a query and yields each row.
 *
 * @param String query
 * @yield Array row
 * @return RubyString either nil or "<error_number>: message"
 */
static VALUE
snowflake_query_interface(VALUE self, VALUE query) {
    rb_need_block();

    SF_CONNECT *snowflake_connection;
    TypedData_Get_Struct(self, SF_CONNECT, &sf_wrapper, snowflake_connection);

    SF_STMT *sfstmt = snowflake_stmt(snowflake_connection);

    SF_STATUS status = snowflake_query(sfstmt, StringValueCStr(query), 0);
    if (status != SF_STATUS_SUCCESS) return dump_error(&(sfstmt->error));

    int64 num_fields = snowflake_num_fields(sfstmt);
    //    rb_warn("num_fields %lld\n", num_fields);
    SF_COLUMN_DESC *descs = snowflake_desc(sfstmt);
    VALUE row[num_fields];

    while ((status = snowflake_fetch(sfstmt)) == SF_STATUS_SUCCESS) {
        int i;
        for (i = 0; i < num_fields; ++i) {
            // universally handle null fields alike
            sf_bool is_null;
            snowflake_column_is_null(sfstmt, i + 1, &is_null);
            if (is_null) {
                row[i] = Qnil;
                continue;
            }
            switch (descs[i].type) {
                case SF_DB_TYPE_FIXED: {
                    int64 int_val;
                    status = snowflake_column_as_int64(sfstmt, i + 1, &int_val);
                    row[i] = LONG2NUM(int_val);
                    break;
                }
                case SF_DB_TYPE_REAL: {
                    float64 value;
                    status = snowflake_column_as_float64(sfstmt, i + 1, &value);
                    row[i] = DBL2NUM(value);
                    break;
                }
                case SF_DB_TYPE_TEXT: {
                    const char *value = NULL;
                    status = snowflake_column_as_const_str(sfstmt, i + 1, &value);
                    row[i] = rb_str_new_cstr(value);
                    break;
                }
                case SF_DB_TYPE_DATE:
                case SF_DB_TYPE_TIMESTAMP_LTZ:
                case SF_DB_TYPE_TIMESTAMP_NTZ:
                case SF_DB_TYPE_TIMESTAMP_TZ:
                case SF_DB_TYPE_TIME: {
                    float64 value;
                    status = snowflake_column_as_universal_time(sfstmt, i + 1, &value);
                    row[i] = DBL2NUM(value);
                    break;
                }
                case SF_DB_TYPE_BOOLEAN: {
                    sf_bool value;
                    status = snowflake_column_as_boolean(sfstmt, i + 1, &value);
                    row[i] = (VALUE) value ? Qtrue : Qfalse;
                    break;
                }
                default:
                    rb_raise(rb_eNotImpError,
                             "Add deserializer for type %d in enum SF_C_TYPE in libsnowflakeclient#client.h",
                             descs[i].type);
            }
            if (status != SF_STATUS_SUCCESS) {
                rb_raise(rb_eIOError, "%d", status);
            }
        }
        rb_yield(rb_ary_new_from_values(num_fields, row));
    }
    if (status != SF_STATUS_EOF) return dump_error(&(sfstmt->error));
    else return Qnil;
}

static VALUE
return_pair(VALUE first, VALUE second) {
    VALUE result = rb_ary_new2(2);
    rb_ary_store(result, 0, first);
    rb_ary_store(result, 1, second);
    return result;
}

/**
 * Executes a query returns number of altered rows
 *
 * @param String insert_or_delete
 * @return ruby integer number of rows, error string (2 sb mutually exclusive values one of which will usually be `nil`)
 */
static VALUE
snowflake_send_change(VALUE self, VALUE insert_or_delete) {
    SF_CONNECT *snowflake_connection;
    TypedData_Get_Struct(self, SF_CONNECT, &sf_wrapper, snowflake_connection);

    SF_STMT *sfstmt = snowflake_stmt(snowflake_connection);
    SF_STATUS status = snowflake_query(sfstmt, StringValueCStr(insert_or_delete), 0);
    if (status != SF_STATUS_SUCCESS) return return_pair(Qnil, dump_error(&(sfstmt->error)));
    else return return_pair(LONG2NUM(snowflake_affected_rows(sfstmt)), Qnil);
}

VALUE
snowflake_connection_get_field(VALUE self, SF_ATTRIBUTE field_name) {
    SF_CONNECT *sf;
    TypedData_Get_Struct(self, SF_CONNECT, &sf_wrapper, sf);

    void *value = NULL;
    snowflake_get_attribute(sf, field_name, &value);
    return rb_str_new_cstr(value);
}

VALUE
snowflake_connection_host(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_HOST);
}

VALUE
snowflake_connection_account(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_ACCOUNT);
}

VALUE
snowflake_connection_warehouse(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_WAREHOUSE);
}

VALUE
snowflake_connection_database(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_DATABASE);
}

VALUE
snowflake_connection_schema(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_SCHEMA);
}

VALUE
snowflake_connection_username(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_USER);
}

VALUE
snowflake_connection_role(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_ROLE);
}

VALUE
snowflake_connection_timezone(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_TIMEZONE);
}

VALUE
snowflake_connection_port(VALUE self) {
    return snowflake_connection_get_field(self, SF_CON_PORT);
}

void
Init_ruby_snowflake_client() {
    rb_cEncapsulateSfConnect = rb_define_class("SnowflakeCConnection", rb_cData);

    rb_define_alloc_func(rb_cEncapsulateSfConnect, sf_wrapper_alloc);

    rb_define_method(rb_cEncapsulateSfConnect, "initialize", sf_wrapper_m_initialize, 10);
    rb_define_method(rb_cEncapsulateSfConnect, "snowflake_query", snowflake_query_interface, 1);
    rb_define_method(rb_cEncapsulateSfConnect, "snowflake_update", snowflake_send_change, 1);

    rb_define_method(rb_cEncapsulateSfConnect, "host", snowflake_connection_host, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "account", snowflake_connection_account, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "warehouse", snowflake_connection_warehouse, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "database", snowflake_connection_database, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "schema", snowflake_connection_schema, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "username", snowflake_connection_username, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "role", snowflake_connection_role, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "timezone", snowflake_connection_timezone, 0);
    rb_define_method(rb_cEncapsulateSfConnect, "port", snowflake_connection_port, 0);
}