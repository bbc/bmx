SET _header=%1
SET _dist_header=%2
SET _define_name=%3

SET "_default_str=#define %_define_name% """

REM if _header already exists then 'touch'
REM else copy the distribution package header _dist_header
REM else create header with empty version string _default_str

IF EXIST %_header% (
  COPY /B %_header% +,, > NUL
) ELSE (
  IF EXIST %_dist_header% (
    COPY %_dist_header% %_header%
  ) ELSE (
    ECHO.%_default_str% > %_header%
  )
)
