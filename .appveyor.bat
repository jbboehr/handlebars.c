
@ECHO ON

set DIRENT_REPO_DIR="%BUILD_CACHE_DIR%\dirent"
set GETOPT_REPO_DIR="%BUILD_CACHE_DIR%\getopt"
set CHECK_REPO_DIR="%BUILD_CACHE_DIR%\check"
set CHECK_REPO_BRANCH="windows"
set CHECK_BUILD_DIR="%BUILD_CACHE_DIR%\check\build"
set JSONC_REPO_DIR="%BUILD_CACHE_DIR%\json-c"
set JSONC_REPO_BRANCH="windows"
set JSONC_BUILD_DIR="%BUILD_CACHE_DIR%\json-c\build"
set PCRE_REPO_DIR="%BUILD_CACHE_DIR%\pcre"
set PCRE_REPO_BRANCH="master"
set PCRE_BUILD_DIR="%BUILD_CACHE_DIR%\pcre\build"
set TALLOC_REPO_DIR="%BUILD_CACHE_DIR%\talloc"
set TALLOC_REPO_BRANCH="master"
set TALLOC_BUILD_DIR="%BUILD_CACHE_DIR%\talloc\build"
set YAML_REPO_DIR="%BUILD_CACHE_DIR%\yaml"
set YAML_REPO_BRANCH="master"
set YAML_BUILD_DIR="%BUILD_CACHE_DIR%\yaml\build"

set HANDLEBARS_SRC_DIR=%APPVEYOR_BUILD_FOLDER%
set HANDLEBARS_BUILD_DIR=%APPVEYOR_BUILD_FOLDER%\cmake-build
set HANDLEBARS_DIR=%APPVEYOR_BUILD_FOLDER%\artifacts

if "%1" == "install" (
	if not exist "%BUILD_CACHE_DIR%" (
		mkdir %BUILD_CACHE_DIR%
	)

	REM dirent.h
	if not exist "%DIRENT_REPO_DIR%" (
		git clone https://github.com/jbboehr/dirent.git %DIRENT_REPO_DIR%
	) else (
		cd %DIRENT_REPO_DIR%
		git fetch origin
		git checkout --force origin/master
	)
	echo %DIRENT_REPO_DIR%\include\dirent.h %ARTIFACT_DIR%\include
	copy /Y %DIRENT_REPO_DIR%\include\dirent.h %ARTIFACT_DIR%\include

	REM getopt.h
	if not exist "%GETOPT_REPO_DIR%" (
		git clone https://github.com/jbboehr/Getopt-for-Visual-Studio.git %GETOPT_REPO_DIR%
	) else (
		cd %DIRENT_REPO_DIR%
		git fetch origin
		git checkout --force origin/master
	)
	echo %GETOPT_REPO_DIR%\getopt.h %ARTIFACT_DIR%\include
	copy /Y %GETOPT_REPO_DIR%\getopt.h %ARTIFACT_DIR%\include

	REM check
	if not exist "%CHECK_REPO_DIR%" (
		git clone -b %CHECK_REPO_BRANCH% https://github.com/jbboehr/check.git %CHECK_REPO_DIR%
	) else (
		cd %CHECK_REPO_DIR%
		git fetch origin
		git checkout --force origin/%CHECK_REPO_BRANCH%
	)
	mkdir %CHECK_BUILD_DIR%
	cd %CHECK_BUILD_DIR%
	cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%ARTIFACT_DIR% ..
	nmake all install

	REM json-c
	if not exist "%JSONC_REPO_DIR%" (
		git clone -b %JSONC_REPO_BRANCH% https://github.com/jbboehr/json-c.git %JSONC_REPO_DIR%
	) else (
		cd %JSONC_REPO_DIR%
		git fetch origin
		git checkout --force origin/%JSONC_REPO_BRANCH%
	)
	mkdir %JSONC_BUILD_DIR%
	cd %JSONC_BUILD_DIR%
	cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%ARTIFACT_DIR% ..
	nmake all install

	REM pcre
	if not exist "%PCRE_REPO_DIR%" (
		git clone -b %PCRE_REPO_BRANCH% https://github.com/jbboehr/pcre.git %PCRE_REPO_DIR%
	) else (
		cd %PCRE_REPO_DIR%
		git fetch origin
		git checkout --force origin/%PCRE_REPO_BRANCH%
	)
	mkdir %PCRE_BUILD_DIR%
	cd %PCRE_BUILD_DIR%
	cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%ARTIFACT_DIR% ..
	nmake all install

	REM talloc
	if not exist "%TALLOC_REPO_DIR%" (
		git clone -b %TALLOC_REPO_BRANCH% https://github.com/jbboehr/talloc-win.git %TALLOC_REPO_DIR%
	) else (
		cd %TALLOC_REPO_DIR%
		git fetch origin
		git checkout --force origin/%TALLOC_REPO_BRANCH%
	)
	mkdir %TALLOC_BUILD_DIR%
	cd %TALLOC_BUILD_DIR%
	cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%ARTIFACT_DIR% ..
	nmake all install

	REM yaml
	if not exist "%YAML_REPO_DIR%" (
		git clone -b %YAML_REPO_BRANCH% https://github.com/yaml/libyaml.git %YAML_REPO_DIR%
	) else (
		cd %YAML_REPO_DIR%
		git fetch origin
		git checkout --force origin/%YAML_REPO_BRANCH%
	)
	mkdir %YAML_BUILD_DIR%
	cd %YAML_BUILD_DIR%
	cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%ARTIFACT_DIR% ..
	nmake
	copy /Y *.lib %ARTIFACT_DIR%\lib
	copy /Y ..\include\yaml.h %ARTIFACT_DIR%\include
)

if "%1" == "build_script" (
    mkdir %HANDLEBARS_BUILD_DIR%
    cd %HANDLEBARS_BUILD_DIR%
    cmake -G "NMake Makefiles" -D_CRT_SECURE_NO_WARNINGS=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_LIBRARY_PATH=%ARTIFACT_DIR%\lib -DCMAKE_INCLUDE_PATH=%ARTIFACT_DIR%\include -DCMAKE_INSTALL_PREFIX=%ARTIFACT_DIR% ..
    nmake
    nmake install
)

cd %APPVEYOR_BUILD_FOLDER%
