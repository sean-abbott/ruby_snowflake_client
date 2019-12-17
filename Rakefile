require "bundler/gem_tasks"
require "rubygems/package_task"
require "rake/extensiontask"
require "rake/clean"


CLEAN.include(
  "ext/ruby_snowflake_client/*.o",
  "ext/ruby_snowflake_client/*.bundle"
)

CLOBBER.include(
  "ext/ruby_snowflake_client/Makefile",
  "pkg"
)

BUILD_DIR = 'build'

def gem_spec
    @gem_spec ||= Gem::Specification.load('ruby_snowflake_client.gemspec')
end

Gem::PackageTask.new(gem_spec) do |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
end

Rake::ExtensionTask.new('ruby_snowflake_client', gem_spec) do |ext|
   ext.ext_dir = './ext/ruby_snowflake_client'
   ext.tmp_dir = BUILD_DIR
   ext.config_script = "extconf.rb"
end

task :build_libsnowflakeclient do
    system("./build_libsnowflake.sh")
end

task :build   => [:clean, :compile]

task :default => [:build]
