@Echo off

REM INITIALIZE ANACONDA PYTHON NEXT LINE (requires activate.bat in PATH)
REM CALL activate.bat

START "BpSink" /D "%HDTN_BUILD_ROOT%" "cmd /k" "%HDTN_BUILD_ROOT%\common\bpcodec\apps\bpsink-async.exe" "--my-uri-eid=ipn:2.1" "--inducts-config-file=%HDTN_SOURCE_ROOT%\tests\config_files\inducts\bpsink_one_tcpcl_port4558.json"
timeout /t 3
START "BpGen" /D "%HDTN_BUILD_ROOT%" "cmd /k" "%HDTN_BUILD_ROOT%\common\bpcodec\apps\bpgen-async.exe" "--force-disable-custody" "--bundle-rate=0" "--my-uri-eid=ipn:1.1" "--dest-uri-eid=ipn:2.1" "--duration=20" "--bundle-size=100000" "--custody-transfer-inducts-config-file=%HDTN_SOURCE_ROOT%\tests\config_files\inducts\bpsink_one_tcpcl_port4557.json"
timeout /t 6
START "HDTN One Process" /D "%HDTN_BUILD_ROOT%" "cmd /k" "%HDTN_BUILD_ROOT%\module\hdtn_one_process\hdtn-one-process.exe" "--cut-through-only-test" "--hdtn-config-file=%HDTN_SOURCE_ROOT%\tests\config_files\hdtn\hdtn_egress2tcpcl_port4557flowid1_port4558flowid2.json"