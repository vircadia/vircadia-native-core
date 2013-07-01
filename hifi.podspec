#
# Be sure to run `pod spec lint hifi.podspec' to ensure this is a
# valid spec and remove all comments before submitting the spec.
#
# To learn more about the attributes see http://docs.cocoapods.org/specification.html
#
Pod::Spec.new do |s|
  s.name         = "hifi"
  s.version      = "0.0.1"
  s.summary      = "Test platform for various render and interface tests for next-gen VR system."

  s.homepage     = "https://github.com/worklist/hifi"

  # Specify the license type. CocoaPods detects automatically the license file if it is named
  # 'LICENCE*.*' or 'LICENSE*.*', however if the name is different, specify it.
  # s.license      = 'MIT (example)'

  # Specify the authors of the library, with email addresses. You can often find
  # the email addresses of the authors by using the SCM log. E.g. $ git log
  #
  s.author       = { "Worklist" => "contact@worklist.net" }

  # Specify the location from where the source should be retrieved.
  #
  s.source       = { :git => "https://github.com/worklist/hifi.git", :commit => "ddc85272b939922c32e6a80f763e6de3cb00ab1a" }
  
  s.platform = :ios
  s.ios.deployment_target = "6.0"

  # A list of file patterns which select the source files that should be
  # added to the Pods project. If the pattern is a directory then the
  # path will automatically have '*.{h,m,mm,c,cpp}' appended.
  #
  # s.source_files = 'Classes', 'Classes/**/*.{h,m}'
  # s.exclude_files = 'Classes/Exclude'
  
  s.subspec "shared" do |sp|
    sp.source_files = "libraries/shared/src"
    sp.public_header_files = "librares/shared/src"
    sp.exclude_files = "libraries/shared/src/UrlReader.*"
  end
  
  s.subspec "audio" do |sp|
    sp.source_files = "libraries/audio/src"
    sp.public_header_files = "libraries/audio/src"
    sp.xcconfig = { 'CLANG_CXX_LIBRARY' => "libc++" }
    sp.dependency 'glm'
  end

  # A list of file patterns which select the header files that should be
  # made available to the application. If the pattern is a directory then the
  # path will automatically have '*.h' appended.
  #
  # If you do not explicitly set the list of public header files,
  # all headers of source_files will be made public.
  #
  # s.public_header_files = 'Classes/**/*.h'

  # A list of paths to preserve after installing the Pod.
  # CocoaPods cleans by default any file that is not used.
  # Please don't include documentation, example, and test files.
  #
  # s.preserve_paths = "FilesToSave", "MoreFilesToSave"

  # Specify a list of frameworks that the application needs to link
  # against for this Pod to work.
  #
  # s.framework  = 'SomeFramework'
  # s.frameworks = 'SomeFramework', 'AnotherFramework'

  # Specify a list of libraries that the application needs to link
  # against for this Pod to work.
  #
  # s.library   = 'iconv'
  # s.libraries = 'iconv', 'xml2'

  # If this Pod uses ARC, specify it like so.
  #
  s.requires_arc = false

  # If you need to specify any other build settings, add them to the
  # xcconfig hash.
  #
  # s.xcconfig = { 'CLANG_CXX_LIBRARY' => "libc++" }

  # Finally, specify any Pods that this Pod depends on.
  #
  # s.dependency 'JSONKit', '~> 1.4'
end
