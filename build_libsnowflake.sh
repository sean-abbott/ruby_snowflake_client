#!/bin/bash
# TODO fix the below to work if env var not set
LIB_SNOWFLAKE_DIR = "${LIB_SNOWFLAKE_DIR:-/libsnowflakeclient}"

$LIB_SNOWFLAKE_DIR/scripts/build_libsnowflakeclient.sh -s -t Release
find $LIB_SNOWFLAKE_DIR -name *.a | xargs -I {} cp {} ./lib
cp $LIB_SNOWFLAKE_DIR/include/snowflake/{client.h,logger.h,platform.h,basic_types.h,Simba_CRTFunctionSafe.h,version.h} ./lib
cp $LIB_SNOWFLAKE_DIR/lib/cJSON.h ./lib
