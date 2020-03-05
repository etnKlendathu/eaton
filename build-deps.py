#!/usr/bin/python3
import os
import subprocess

THIS_DIR         = os.path.abspath(os.path.dirname(__file__))
COMPILER_CC      = "clang-9"
COMPILER_CXX     = "clang++-9"
BUILD_GENERATOR  = "Ninja"
BUILD_TYPE       = "Debug"

DEPENDENCIES_DIR = os.path.join(THIS_DIR, "3rdParty")
INSTALL_PREFIX   = os.path.join(THIS_DIR, "runtime", BUILD_TYPE)


# Order is matter :(
DEPENDENCIES = [
    #"catch2",
    #"cxxtools",
    "czmq",
    "libcidr",
    "libsodium",
    "libzmq",
    "cppzmq",
    "log4cplus",
    "malamute",
    "protobuf",
    "yaml"
]

def buildCmake(depPath):
    envir = os.environ
    envir["CC"] = COMPILER_CC
    envir["CXX"] = COMPILER_CXX
    envir["PKG_CONFIG_PATH"] = os.path.join(INSTALL_PREFIX, "lib", "pkgconfig")

    buildPath = os.path.join(depPath, "build", BUILD_TYPE)
    subprocess.run(["mkdir", "-p", os.path.join("build", BUILD_TYPE)], cwd=depPath)
    subprocess.run(["cmake",
        "-G", BUILD_GENERATOR,
        "-DCMAKE_INSTALL_PREFIX="+INSTALL_PREFIX,
        "-DCMAKE_BUILD_TYPE="+BUILD_TYPE,
        "../.."], cwd=buildPath, env = envir)

    subprocess.run(["cmake", "--build", "."], env = envir, cwd=buildPath)


def buildDependencies():
    for dep in DEPENDENCIES:
        buildCmake(os.path.join(DEPENDENCIES_DIR, dep))

if __name__ == "__main__":
    buildDependencies()
