class Bmx < Formula
  desc "Read and write MXF broadcasting media files"
  homepage "https://github.com/bbc/bmx"
  license "BSD-3-Clause"
  # There is an issue with bmx-1.1.tar.gz on Linux Homebrew which is why this is
  # put into a macos only section until a new release is made
  on_macos do
    url "https://github.com/bbc/bmx/releases/download/v1.1/bmx-1.1.tar.gz"
    sha256 "6d4a32a78af9da64b345bb7ed42375e1d9aa4d89263f3334cc5cfd5173e3b645"
  end
  head "https://github.com/bbc/bmx.git", branch: "main"

  livecheck do
    url "https://github.com/bbc/bmx/releases?q=prerelease%3Afalse"
    regex(%r{href=["']?[^"' >]*?/tag/v(\d+\.\d+)["' >]}i)
    strategy :page_match
  end

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  on_linux do
    # This was needed to avoid build failures because there is a mismatch between homebrew glibc
    # that gets installed and the host's C library headers
    depends_on "gcc@11" => :build
  end

  uses_from_macos "expat"
  uses_from_macos "curl"
  depends_on "uriparser"

  def install
    system "cmake", "-S", ".", "-B", "build", "-DBMX_BUILD_WITH_LIBCURL=ON", *std_cmake_args
    system "cmake", "--build", "build"
    system "ctest", "--test-dir", "build", "--output-on-failure"
    system "cmake", "--install", "build"
  end

  test do
    assert_match "bmxtranswrap, bmx", shell_output("#{bin}/bmxtranswrap --version", 0)
  end
end
