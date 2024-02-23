# echoclient만 파일명에 맞게 바꿔주면 됨.
TARGET = hostinfo
OBJS = 	hostinfo.o csapp.o
%.o: %.c
	gcc -c $<
$(TARGET) : $(OBJS)
	gcc -o $(TARGET) $(OBJS)
clean:
	rm -f *.o
	rm -f $(TARGET)