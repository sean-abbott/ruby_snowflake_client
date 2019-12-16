require "bundler/gem_tasks"
require "rubygems/package_task"
require "rake/extensiontask"
require "rake/clean"

CLEAN.include(
  "ext/snowflake_c_connect/*.o",
  "ext/snowflake_c_connect/*.bundle"
)

CLOBBER.include(
  "ext/snowflake_c_connect/Makefile",
  "pkg"
)

BUILD_DIR = 'build'

def gem_spec
    @gem_spec ||= Gem::Specification.load('snowflake_c_connect.gemspec')
end

Gem::PackageTask.new(gem_spec) do |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
end

Rake::ExtensionTask.new('snowflake_c_connect', gem_spec) do |ext|
   ext.ext_dir = './ext/snowflake_c_connect'
   ext.tmp_dir = BUILD_DIR
   ext.config_script = "extconf.rb"
end

task :build   => [:clean, :compile]

task :default => [:build]
