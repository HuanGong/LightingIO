

ADD_EXECUTABLE(log_metrics_test
  component/log_metrics_test.cc
)

TARGET_LINK_LIBRARIES(log_metrics_test
  PUBLIC ltio
  ${LINK_DEP_LIBS}
  ${LtIO_LINKER_LIBS}
  ${LIBUNWIND_LIBRARIES}
)

