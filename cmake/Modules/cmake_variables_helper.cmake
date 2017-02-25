# This file contains helper macro to print env variables as status messages

# print_variable
#
# Prints a status message with the value of the variable
#
# Parameters:
#  variableName - A string containing the name of the variable to be printed
#
# Usage:
#  print_variable(CMAKE_CURRENT_BINARY_DIR)
#
# Output:
#  -- CMAKE_CURRENT_BINARY_DIR=/home/xxx/xxx
#
macro(print_variable _variableName)
  message(STATUS "${_variableName}=${${_variableName}}")
endmacro()

# print_all_variables
#
# Prints a status message for all currently defined variables.
#
# Parameters:
#  none
#
# Usage:
#  print_all_variable()
#
# Output:
#  -- ------------------------------ print variables ------------------------------
#  -- CMAKE_CURRENT_BINARY_DIR=/home/xxx/xxx
#  -- ....
#  -- -----------------------------------------------------------------------------
#
macro(print_all_variables)
  message(STATUS "------------------------------ print variables ------------------------------")
  get_cmake_property(_variableNames VARIABLES)

  foreach(_variableName ${_variableNames})
    print_variable(${_variableName})
  endforeach()
  message(STATUS "-----------------------------------------------------------------------------")
endmacro()
