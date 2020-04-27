#!/usr/bin/python3
import os
import subprocess
import argparse

THIS_DIR         = os.path.abspath(os.path.dirname(__file__))
COMPILER_CC      = "clang-10"
COMPILER_CXX     = "clang++-10"
BUILD_GENERATOR  = "Ninja"
BUILD_TYPE       = "Debug"

DEPENDENCIES_DIR = os.path.join(THIS_DIR, "3rdParty")
INSTALL_PREFIX   = os.path.join(THIS_DIR, "runtime", BUILD_TYPE)


# Order is matter :(
DEPENDENCIES = [
    "catch2",
    "cxxtools",
    "libzmq",
    "czmq",
    "libcidr",
    "libsodium",
    "cppzmq",
    "log4cplus",
    "malamute",
    "protobuf",
    "yaml",
    "tntdb",
    "libmagic",
    "fty-common-logging",
    "fty-common",
    "fty-asset-activator",
    "fty-common-db",
    "fty-common-mlm",
    "fty-proto",
    "fty-common-socket",
    "fty-common-messagebus",
    "fty-common-dto",
    "fty-lib-certificate",
    "fty-security-wallet",
    "fty-common-nut",
    "tntnet",
    "fty-common-rest",
    "fty-shm"
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

    subprocess.run(["cmake", "--build", "."], env = envir, cwd=buildPath).check_returncode()

def buildDependencies(libs = {}):
    for dep in libs if libs else DEPENDENCIES:
        buildCmake(os.path.join(DEPENDENCIES_DIR, dep))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Build libraries.')
    parser.add_argument('lib', nargs='*', help='lib to build')
    args = parser.parse_args()
    if not args.lib:
        buildDependencies()
    else:
        buildDependencies(args.lib)
