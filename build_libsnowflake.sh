#!/bin/bash

RUBY_SNOWFLAKE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
LIB_SNOWFLAKE_DIR="${LIB_SNOWFLAKE_DIR:-$RUBY_SNOWFLAKE_DIR/../libsnowflakeclient}"
CURDIR = $PWD

# zlib static lib isn't compiled by build_dependencies and when linked later says it should have been compiled w fPIC
cd $LIB_SNOWFLAKE_DIR/deps/zlib-1.2.11  # zlib build gets confused if it's not pwd
source configure --static  # may add CFLAGS="-fPIC" but this looks like it does it
source make
cd $CURDIR

#source $LIB_SNOWFLAKE_DIR/scripts/build_dependencies.sh -t Release
source $LIB_SNOWFLAKE_DIR/scripts/build_libsnowflakeclient.sh -s -t Release

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
