from conan import ConanFile
from conan.tools.cmake import cmake_layout


class MessangerServer(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("grpc/1.83.0")
        self.requires("boost/1.89.0")
        self.requires("openssl/3.5.4")
        self.requires("spdlog/1.17.0")

    def layout(self):
        cmake_layout(self)