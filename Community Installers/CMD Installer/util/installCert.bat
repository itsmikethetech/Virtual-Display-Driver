@echo off

set CERTIFICATE="%~dp0VDD.cer"

certutil -addstore -f root %CERTIFICATE%
certutil -addstore -f TrustedPublisher %CERTIFICATE%
pause
