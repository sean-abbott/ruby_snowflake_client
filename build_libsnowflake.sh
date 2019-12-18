#!/bin/bash

RUBY_SNOWFLAKE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
LIB_SNOWFLAKE_DIR="${LIB_SNOWFLAKE_DIR:-$RUBY_SNOWFLAKE_DIR/../libsnowflakeclient}"

$LIB_SNOWFLAKE_DIR/scripts/build_libsnowflakeclient.sh -s -t Release

if [[ "$OSTYPE" == "darwin"* ]]; then
  PLATFORM="darwin"
else
  PLATFORM="linux"
fi

mkdir -p $RUBY_SNOWFLAKE_DIR/lib
cp $LIB_SNOWFLAKE_DIR/cmake-build/libsnowflakeclient.a $RUBY_SNOWFLAKE_DIR/lib
find $LIB_SNOWFLAKE_DIR/deps-build/$PLATFORM -name *.a | xargs -I {} cp {} $RUBY_SNOWFLAKE_DIR/lib
cp $LIB_SNOWFLAKE_DIR/include/snowflake/{client.h,logger.h,platform.h,basic_types.h,Simba_CRTFunctionSafe.h,version.h} $RUBY_SNOWFLAKE_DIR/lib
cp $LIB_SNOWFLAKE_DIR/lib/cJSON.h $RUBY_SNOWFLAKE_DIR/lib
