# ruby_snowflake_client

A small gem wrapping [libsnowflakeclient](https://github.com/snowflakedb/libsnowflakeclient) whose goal is to 
stream query results so as to not allocate memory for more than a row at a time.

Build:
`rake build`

Usage:
``` ruby
# may raise an IoError if it fails to instantiate or connect
connection = SnowflakeCConnection.new(host, account, 'WH', database, schema, user, password, 'SERVICE_ROLE', timezone_or_nil, port_or_nil)
query = <<~QUERY
  select account_id, order_time, amount 
    from orders where order_time < #{4.hours.ago}
      and shipping_time is null
    order by order_time
QUERY

# read query
status = connection.snowflake_query(query) do |row|
  process_overdue_order(row[0], # account
                        Time.at(row[1]), # order time--all dates and times come back as seconds since epoch
                        row[1])
end
raise status if status  # status either `nil` or sprintf(msg, "%d: %s", error->error_code, error->msg);

# update query (can use for any query where you only care about the number of affected rows)
rows, status = connection.snowflake_update("delete from orders where status = 'cancelled' and order_time < #{1.year.ago}")
if rows
  puts "Deleted #{rows} cancelled orders from over a year ago"
elsif status
  raise status  # error_code: error_msg
end

```