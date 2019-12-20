Gem::Specification.new do |s|
  s.name    = "ruby_snowflake_client"
  s.version = "0.1.1"
  s.summary = "Snowflake connect for Ruby"
  s.author  = "CarGurus"
  s.email   = 'dmitchell@cargurus.com'
  s.platform = Gem::Platform::CURRENT
  s.description = <<~DESC
    Uses the c static library libsnowflakeclient to connect to and communicate with Snowflake. That library uses curl.
    This library is much faster than using ODBC especially for large result sets and avoids ODBC butchering of timezones.
  DESC
  s.license = 'MIT'  # TODO double check

  # do we really want all these sources in the gem?
  s.files = Dir.glob("ext/**/*.{c,rb}") +
            Dir.glob("lib/**/*.{a,h,rb}")

  # perhaps nothing and figure out how to build and pkg the platform specific .so, or .a, or ...
  s.extensions << "ext/ruby_snowflake_client/extconf.rb"

  s.add_development_dependency "bundler"
  s.add_development_dependency "rake"
  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "rubygems-tasks"
end