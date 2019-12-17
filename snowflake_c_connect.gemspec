Gem::Specification.new do |s|
  s.name    = "ruby_snowflake_connect"
  s.version = "0.1.0"
  s.summary = "Snowflake connect for Ruby"
  s.author  = "CarGurus"

  s.files = Dir.glob("ext/**/*.{c,rb}") +
            Dir.glob("lib/**/*.{a,rb}")

  s.extensions << "ext/snowflake_c_connect/extconf.rb"

  s.add_development_dependency "bundler"
  s.add_development_dependency "rake"
  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "rspec"
  s.add_development_dependency "rubygems-tasks"
end