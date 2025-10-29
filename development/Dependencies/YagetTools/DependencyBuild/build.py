import argparse
from os import path
import os
import filecmp
import shutil
import json
import pprint
import copy
import subprocess

from os.path import abspath

deployTag = 'BUILD'

# Load actuall meta configuration file (.build)
# from disk and return json object or None
def LoadMetaFile(fileName):
    try:
        with open(fileName, 'r') as f:
            try:
                datastore = json.load(f)
                return datastore

            except json.JSONDecodeError as jex:
                print("** DEPLOY ERROR **")
                print("Invalid build file:\n{}({}, {}): error {}".format(ConvertPath(fileName), jex.lineno, jex.colno, jex))
                lines = jex.doc.splitlines()
                if jex.lineno > -1 and jex.lineno < len(lines):
                    wrongCodeLine = lines[jex.lineno]
                    print("Offending line (between * *):")
                    print("     *{}*".format(wrongCodeLine))
                    colNo = max(1, jex.colno)
                    print("      {}".format('-' * (colNo - 1) + '!'))

                return None
    except IOError:
        return None

def ConvertPath(path):
    return path.replace(os.path.sep, '/')

#----------------------------------------------------------------------------------------------------
#---------------------------- assimp -------------------------------------------
# Dependencies> git clone --recursive https://github.com/assimp/assimp.git assimp-5.0.1
# Dependencies> rmdir /S /Q assimp-5.0.1\.git   ; check first for existance if this filter (hidden)
# Dependencies> mkdir assimp-5.0.1\builds       ; delete old builds folder if exist
# Dependencies> cd assimp5.0.1\builds
#
# Dependencies/assimp-5.0.1/builds> cmake ../ -G"Visual Studio 16 2019" -A x64 -DASSIMP_BUILD_ZLIB=ON
# Dependencies/assimp-5.0.1/builds> cmake --build . --config Debug
# Dependencies/assimp-5.0.1/builds> bin\Debug\unit.exe
# Dependencies/assimp-5.0.1/builds> cmake --build . --config Release
# Dependencies/assimp-5.0.1/builds> bin\Release\unit.exe


#----------------------------------------------------------------------------------------------------
#---------------------------- flatbuffers -------------------------------------------
# Dependencies> git clone --recursive https://github.com/google/flatbuffers.git flatbuffers-1.11.0
# Dependencies> rmdir /S /Q flatbuffers-1.11.0\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir flatbuffers-1.11.0\builds      ; delete old builds folder if exist
# Dependencies> cd flatbuffers-1.11.0\builds
#
# Dependencies/flatbuffers-1.11.0/builds> cmake ../ -G"Visual Studio 16 2019" -A x64
# Dependencies/flatbuffers-1.11.0/builds> cmake --build . --config Debug
# Dependencies/flatbuffers-1.11.0/builds> Debug\flattests.exe
# Dependencies/flatbuffers-1.11.0/builds> cmake --build . --config Release
# Dependencies/flatbuffers-1.11.0/builds> Release\flattests.exe


#----------------------------------------------------------------------------------------------------
#---------------------------- DirectXTex -------------------------------------------
# Dependencies> git clone --recursive https://github.com/microsoft/DirectXTex.git DirectXTex-170
# Dependencies> rmdir /S /Q DirectXTex-170\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir DirectXTex-170\builds      ; delete old builds folder if exist
# Dependencies> cd DirectXTex-170\builds
#
# Dependencies/DirectXTex-170/builds> cmake ../ -G"Visual Studio 16 2019" -A x64
# Dependencies/DirectXTex-170/builds> cmake --build . --config Debug
# Dependencies/DirectXTex-170/builds> cmake --build . --config Release


#----------------------------------------------------------------------------------------------------
#---------------------------- DirectXTK11 -------------------------------------------
# Dependencies> git clone --recursive https://github.com/microsoft/DirectXTK.git DirectXTK11-5.10.2020
# Dependencies> rmdir /S /Q DirectXTK11-5.10.2020\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir DirectXTK11-5.10.2020\builds      ; delete old builds folder if exist
# Dependencies> cd DirectXTK11-5.10.2020\builds
#
# Dependencies/DirectXTK11-5.10.2020/builds> cmake ../ -G"Visual Studio 16 2019" -A x64
# Dependencies/DirectXTK11-5.10.2020/builds> cmake --build . --config Debug
# Dependencies/DirectXTK11-5.10.2020/builds> cmake --build . --config Release


#----------------------------------------------------------------------------------------------------
#---------------------------- DirectXTK12 -------------------------------------------
# Dependencies> git clone --recursive https://github.com/microsoft/DirectXTK12.git DirectXTK12-4.3.2020
# Dependencies> rmdir /S /Q DirectXTK12-4.3.2020\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir DirectXTK12-4.3.2020\builds      ; delete old builds folder if exist
# Dependencies> cd DirectXTK12-4.3.2020\builds
#
# Dependencies/DirectXTK12-4.3.2020/builds> cmake ../ -G"Visual Studio 16 2019" -A x64
# Dependencies/DirectXTK12-4.3.2020/builds> cmake --build . --config Debug
# Dependencies/DirectXTK12-4.3.2020/builds> cmake --build . --config Release

#----------------------------------------------------------------------------------------------------
#---------------------------- fmt -------------------------------------------
# Dependencies> git clone --recursive https://github.com/fmtlib/fmt.git fmt-6.2.0
# Dependencies> rmdir /S /Q fmt-6.2.0\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir fmt-6.2.0\builds      ; delete old builds folder if exist
# Dependencies> cd fmt-6.2.0\builds
#
# Dependencies/fmt-6.2.0/builds> cmake ../ -G"Visual Studio 16 2019" -A x64
# Dependencies/fmt-6.2.0/builds> cmake --build . --config Debug
# Dependencies/fmt-6.2.0/builds> bin\Debug\format-test.exe
# Dependencies/fmt-6.2.0/builds> cmake --build . --config Release
# Dependencies/fmt-6.2.0/builds> bin\Release\format-test.exe


#----------------------------------------------------------------------------------------------------
#---------------------------- nlohmann-json -------------------------------------------
# Dependencies> git clone --recursive https://github.com/nlohmann/json.git nlohmann-json-3.7.3
# Dependencies> rmdir /S /Q nlohmann-json-3.7.3\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir nlohmann-json-3.7.3\builds      ; delete old builds folder if exist
# Dependencies> cd nlohmann-json-3.7.3\builds
#
# Dependencies/nlohmann-json-3.7.3/builds> cmake ../ -G"Visual Studio 16 2019" -A x64
# Dependencies/nlohmann-json-3.7.3/builds> cmake --build . --config Debug
# Dependencies/nlohmann-json-3.7.3/builds> ctest --output-on-failure -C Debug   ; does not work
# Dependencies/nlohmann-json-3.7.3/builds> cmake --build . --config Release
# Dependencies/nlohmann-json-3.7.3/builds> ctest --output-on-failure -C Release ; does not work

#------------------------------------ Evaluations  --------------------------------------------------
#----------------------------------------------------------------------------------------------------
#---------------------------- reactphysics3d -------------------------------------------
# Dependencies> git clone --recursive https://github.com/DanielChappuis/reactphysics3d.git reactphysics3d-0.7.1
# Dependencies> rmdir /S /Q reactphysics3d-0.7.1\.git  ; check first for existance if this filter (hidden)
# Dependencies> mkdir reactphysics3d-0.7.1\builds      ; delete old builds folder if exist
# Dependencies> cd reactphysics3d-0.7.1\builds
#
# Dependencies/reactphysics3d-0.7.1/builds> cmake ../ -G"Visual Studio 16 2019" -A x64 -DRP3D_COMPILE_TESTBED=ON -DRP3D_COMPILE_TESTS=ON -DRP3D_PROFILING_ENABLED=ON -DRP3D_LOGS_ENABLED=ON
# Dependencies/reactphysics3d-0.7.1/builds> cmake --build . --config Debug
# Dependencies/reactphysics3d-0.7.1/builds> test\Debug\tests.exe
# Dependencies/reactphysics3d-0.7.1/builds> cmake --build . --config Release
# Dependencies/reactphysics3d-0.7.1/builds> test\Release\tests.exe

#----------------------------------------------------------------------------------------------------
#---------------------------- PhysX -------------------------------------------
# Dependencies> git clone --recursive https://github.com/NVIDIAGameWorks/PhysX.git PhysX-4.1
# Dependencies> rmdir /S /Q PhysX-4.1\.git  ; check first for existance if this filter (hidden)
# Dependencies> cd PhysX-4.1\physx
#
# Dependencies/PhysX-4.1/physx> generate_projects.bat   ; will ask for msdev version selection
# Dependencies/PhysX-4.1/physx> cd compiler\vc16win64
# Dependencies/PhysX-4.1/physx/compiler\vc16win64>
# 	'create' include\typeinfo.h
# 	include <typeinfo>
# 	Change ''Treat Warnings As Errors' to No (/WX-) for Sample Renderer (All Configurations)
# Dependencies/PhysX-4.1/physx/compiler\vc16win64> devenv PhysXSDK.sln /build Debug
# Dependencies/PhysX-4.1/physx/compiler\vc16win64> devenv PhysXSDK.sln /build Release


#----------------------------------------------------------------------------------------------------
#---------------------------- bullet3 -------------------------------------------
# Dependencies> git clone --recursive https://github.com/bulletphysics/bullet3.git bullet3-2.8.9
# Dependencies> rmdir /S /Q bullet3-2.8.9\.git  ; check first for existance if this filter (hidden)
# Dependencies> cd bullet3-2.8.9


# git clone <--recursive> https_address <optional_folder_name> <--branch branch_name>
# cmake --build . --target MyExe --config Debug

deployTag = 'MODULE-BUILD'


def BuildDependency(title, jsonBlock):

    print(title + ':')
    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint(jsonBlock)


class ModuleBuilder:
    """Represents module configuration and build rules.
    It is initialized from json blob with defaults"""

    def __init__(self, name, dependencyFolder, jsonBlock, defaultBlock, silent, displayMode):
        self.Name = name
        self.DependencyFolder =  dependencyFolder
        self.Version = '{}'.format(jsonBlock['version'])

        if len(self.Version) > 0:
            self.RootFolderName = '{}-{}'.format(name, self.Version)
        else:
            self.RootFolderName = '{}'.format(name)

        self.PrintProgress = silent == False
        self.SupressTests = False
        self.DisplayMode =  displayMode

        cmakeDefaults = {}
        if 'cmake' in defaultBlock:
            cmakeDefaults = defaultBlock['cmake']

        if 'cmake' in jsonBlock and 'build_folder' in jsonBlock['cmake']:
            self.BuildFolderName = path.join(self.RootFolderName, jsonBlock['cmake']['build_folder'])
        elif 'build_folder' in cmakeDefaults:
            self.BuildFolderName = path.join(self.RootFolderName, cmakeDefaults['build_folder'])
        else:
            self.BuildFolderName = self.RootFolderName

        self.BuildFolderName =  path.join(self.DependencyFolder, self.BuildFolderName)

        self.BuildOptions = []
        if 'options' in cmakeDefaults:
            self.BuildOptions = cmakeDefaults['options']

        if 'cmake' in jsonBlock and 'options' in jsonBlock['cmake']:
            self.BuildOptions.extend(jsonBlock['cmake']['options'])

        cmakeCompileCommands =  ['-GVisual Studio 16 2019', '-Ax64']
        if 'cmakeCompileCommands' in defaultBlock:
            cmakeCompileCommands = defaultBlock['cmakeCompileCommands']

        self.BuildConfigurations = []
        if 'configurations' in defaultBlock:
            self.BuildConfigurations = copy.deepcopy(defaultBlock['configurations'])

        if 'configurations' in jsonBlock:
            self.BuildConfigurations.extend(jsonBlock['configurations'])

        self.CmakeCommand = 'cmake'
        self.CmakeOptions = [ '../']
        self.CmakeOptions.extend(cmakeCompileCommands)
        self.CmakeOptions.extend(self.BuildOptions)

        self.BuildCommands = []
        for configuration in self.BuildConfigurations:
            self.BuildCommands.append('cmake --build . --config {}'.format(configuration))

        self.TestCommands = []
        if 'tests' in defaultBlock:
            self.TestCommands = copy.deepcopy(defaultBlock['tests'])

        if 'tests' in jsonBlock:
            self.TestCommands.extend(jsonBlock['tests'])

        if 'options' in jsonBlock and 'suppress_tests' in jsonBlock['options']:
            self.SupressTests = True

    def __repr__(self):
        return '<class \'{}\' instance at {}>\nName: {}\nVersion: {}\nRoot Folder: {}\nBuild Folder: {}\nBuild options: {}\nBuild Configurations: {}\nGenerator: {}{}'.format(
            self.__class__.__name__, id(self), self.Name, self.Version, self.RootFolderName, self.BuildFolderName, self.BuildOptions, self.BuildConfigurations, self.CmakeCommand, self.CmakeOptions)

    def Configure(self, clean):
        '''Configure build environment and generate platform specific project'''

        isFolder = os.path.isdir(self.BuildFolderName)

        if not self.DisplayMode:
            if clean:
                if isFolder:
                    try:
                        shutil.rmtree(self.BuildFolderName)
                        isFolder = False;
                    except OSError as e:
                        print("Error: %s : %s" % (self.BuildFolderName, e.strerror))
                        return False

            if not isFolder:
                os.makedirs(self.BuildFolderName)
        else:
            print("Project: '{}', configuring builds folder: '{}', exist: '{}'.".format(self.Name, self.BuildFolderName, isFolder))

        configureCommand = [self.CmakeCommand]
        configureCommand.extend(self.CmakeOptions)
        self.ExecuteCommand(configureCommand, self.BuildFolderName, self.PrintProgress)

        return True

    def Build(self):
        '''Build dependency module based on configuration run'''
        if not self.DisplayMode:
            if not os.path.isdir(self.BuildFolderName):
                print("Error: Build folder '{}' does not exist".format(self.BuildFolderName))
                return False
        else:
            print("Project '{}' is building configuratians '{}'.".format(self.Name, self.BuildCommands));

        results = 0
        for command in self.BuildCommands:
            results += self.ExecuteCommand(command, self.BuildFolderName, self.PrintProgress);

        return results == len(self.BuildCommands)

    def Tests(self):
        '''Run all unit tests'''
        if  self.SupressTests:
            print("Warning: unit tests for {} are suppressed.".format(self.Name))
            return True;

        if self.DisplayMode:
            print("Project: '{}' running tests: '{}'.".format(self.Name, self.TestCommands));
            return True;

        results = 0
        for test in self.TestCommands:
            fullPath = path.join(self.BuildFolderName, test)
            if os.path.isfile(fullPath):
                results += self.ExecuteCommand(fullPath, self.BuildFolderName, self.PrintProgress)
            else:
                print("Error: Test executable '{}' does not exist".format(fullPath))

        return results == len(self.TestCommands)

    def ExecuteCommand(self, command, workingDir, printProgress):
        '''Execute command as process, pipe output to stdout (if not silent)'''
        if self.DisplayMode:
            print("Executing command: '{}', working directory: '{}'.".format(command, workingDir))
            return True;

        process = subprocess.Popen(command, shell=True, cwd = workingDir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        while True:
            result = process.stdout.readline()
            line = result.decode('utf-8')
            if  line == '\r\n':
                continue

            line = line.rstrip("\n")
            line = line.rstrip("\r")
            if printProgress:
                print(line)

            if result.decode('utf-8') == '' and process.poll() != None:
                if process.returncode != 0:
                    print("Error executing command: '{}'.".format(command))
                    return False

                return True


# build.py --root=c:\Development\yaget\Dependencies --metafile=.\Sample.build
# optional
#   --display
#   --silent
#   --clean
#   --filter=<reg_expresion>

def main():

    global deployTag

    parser = argparse.ArgumentParser(description='''Automation of git, build and deploy of yaget dependency libraries. Yaget (c)2020.
                                                  # Sample input [$(YAGET_ROOT_FOLDER)/DevTools/DependencyDeployment/deploy.py --root=$(YAGET_ROOT_FOLDER) --configuration=$(Configuration) --metafile=$(ProjectDir)$(TargetName).deployment]
                                                  # expended to [c:/Development/yaget/DevTools/DependencyDeployment/deploy.py --root=c:/Development/yaget --configuration=Debug --destination=c:/Development/yaget/branch/version_0_2/bin/Coordinator/x64.Debug/ --metafile=C:/Development/yaget/branch/version_0_2/Research/Coordinator/build/Coordinator.deployment]''')
    parser.add_argument('-r', '--root', dest='root', default='./', help='Root used for prefix to module/dependency folder.')
    parser.add_argument('-m', '--metafile', dest='meta', required=True, help='Json file name (implicitly adds extension .build) which contains list of configurations for how to compile each module/dependency')
    parser.add_argument('-s', '--silent', action='store_true', help='Supress print statements (does not apply to --help or error messages')
    parser.add_argument('-t', '--test_skip', action='store_true', help='Skip all tests')
    parser.add_argument('-f', '--filter', dest='filter', help='Reg expresion for which dependencies to run build(s)')
    parser.add_argument('-c', '--clean', dest='clean', action='store_true', help='Fully rebuild module')
    parser.add_argument('-d', '--display', dest='display', action='store_true', help='Display (print) what would be congfired, build and test.')

    args = parser.parse_args()

    if not path.isfile(args.meta):
        if path.isfile(args.meta + '.build'):
            args.meta += '.build'
        else:
            print("[{}] ERR: Metafile: '{}' is not a valid file.".format(deployTag, ConvertPath(args.meta)))
            exit(1)

    args.root = abspath(args.root)

    if not path.isdir(args.root):
        print("[{}] ERR: root: '{}' is not a valid directory.".format(deployTag, ConvertPath(args.root)))
        exit(1)

    datastore = LoadMetaFile(args.meta)
    if not datastore:
        exit(1)

    pullFromGit = False

    # default sections
    defaultBlock = {}
    if 'Defaults' in datastore:
        defaultBlock = datastore['Defaults']

    cmakeDefaults = {}
    if 'cmake' in defaultBlock:
        cmakeDefaults = defaultBlock['cmake']

    configurationsDefault = []
    if 'configurations' in defaultBlock:
        configurationsDefault = defaultBlock['configurations']

    testsDefault = []
    if 'tests' in defaultBlock:
        testsDefault = defaultBlock['tests']


    modules = {}
    if 'Modules' in datastore:
        modules = datastore['Modules']

    evaluations = {}
    if 'Evaluations' in datastore:
        evaluations = datastore['Evaluations']

    dependenciesResult = True
    generators = []

    moduleNames = modules.keys()
    for name in moduleNames:
        if args.filter == None or name.find(args.filter) != -1:
            jsonBlock = modules[name]
            generator = ModuleBuilder(name, args.root, jsonBlock, defaultBlock, args.silent, args.display)
            generators.append(generator)

    print('[Y] Yaget Build Dependency Tool (c)2020, (c)2023, (c)2025.')
    displayMode = ' In Display Mode' if args.display else ''
    print('[Y] Running configurations{}...'.format(displayMode))

    genreratedProjects = 0
    compiledProjects = 0
    testProjects = 0

    configurePass = []
    for generator in generators:
        print("[Y]     Configuring module: '{}', version: {}...".format(generator.Name, generator.Version))
        if generator.Configure(args.clean):
            configurePass.append(generator)
            genreratedProjects += 1
            print("[Y]     Configured module: '{}', version: {} added.".format(generator.Name, generator.Version))
        else:
            print("[Y]     Module: '{}', version: {} failed to configure.".format(generator.Name, generator.Version))


    print('\n[Y] Running builds...')
    for generator in configurePass:
        result = generator.Build()
        if not result:
            dependenciesResult = False
        compiledProjects += result
        print("[Y]     Build '{}' secessfull: {}.".format(generator.Name, result))

    if args.test_skip:
        print('\n[Y] Tests skipped.')
    else:
        print('\n[Y] Running tests...')
        for generator in configurePass:
            result = generator.Tests()
            if not result:
                dependenciesResult = False
            testProjects += result
            if len(generator.TestCommands) > 0:
                print("[Y]     Tests for '{}' using {} secessfull: {}.".format(generator.Name, generator.TestCommands, result))
            else:
                print("[Y]     No tests specified for '{}', passed: {}.".format(generator.Name, result))

    print("\n[Y] Configured: {}, Builded: {}, Tested: {}. Full Clean Run: {}.".format(genreratedProjects, compiledProjects, testProjects, genreratedProjects == compiledProjects == testProjects))
    print('[Y] Finished dependencies configuration and builds. Result: {}.'.format(dependenciesResult))


if __name__ == "__main__":
    main()

