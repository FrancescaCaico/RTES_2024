[General]
ned-path = .;../queueinglib
network = exam
#cpu-time-limit = 60s
cmdenv-config-name =   examBase
#tkenv-default-config = examBase
qtenv-default-config = examBase
repeat = 5
sim-time-limit = 10000s
#debug-on-errors = true
**.vector-recording = false

[Config examBase]
description = "Global scenario"
**.sink.lifeTime.result-recording-modes = +histogram
**.srv.queueLength.result-recording-modes = +histogram

% for N in [20]:
[Config exam_x${"%03d" % int(N*100)}]
extends = examBase

%endfor

