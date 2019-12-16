# from http://blog.zachallett.com/howto-ruby-c-extension-with-a-static-library/
require 'mkmf'

extension_name = 'ruby_snowflake_connect'

LIBDIR = RbConfig::CONFIG['libdir']
INCLUDEDIR = RbConfig::CONFIG['includedir']
HEADER_DIRS = [INCLUDEDIR, '/libsnowflakeclient']

# setup constant that is equal to that of the file path that holds that static libraries that will need to be compiled against
LIB_DIRS = [LIBDIR, '/libsnowflakeclient']

# array of all libraries that the C extension should be compiled against
libs = ['-lsnowflakeclient', '-laws-cpp-sdk-core', '-laws-cpp-sdk-s3', '-lazure-storage-lite', '-lcrypto', '-lcurl', '-lssl', '-luuid', '-lz']

dir_config('snowflakeclient', HEADER_DIRS, LIB_DIRS)

# iterate though the libs array, and append them to the $LOCAL_LIBS array used for the makefile creation
libs.each do |lib|
  $LOCAL_LIBS << "#{lib} "
end

create_makefile(extension_name)

