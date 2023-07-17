# Start_Alarm(ID) <time> <Category> <Message>
# Start_Alarm(1234) 5 Family Some

compile:
	cc new_alarm_cond.c -D_POSIX_PTHREAD_SEMANTICS -lpthread

run:
	./a.out

start:  compile run
