install(
    TARGETS queue-exploration_exe
    RUNTIME COMPONENT queue-exploration_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
