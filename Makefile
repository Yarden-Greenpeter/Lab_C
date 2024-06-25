CC = gcc
CFlags = -g -Wall -m32

Sources = myshell.c mypipeline.c
OFiles = $(Sources:.c=.o)
Ex = myshell
ExPipeline = mypipeline

all: $(Ex) $(ExPipeline)

$(Ex): $(filter-out mypipeline.o looper.o, $(OFiles))
	$(CC) $(CFlags) $^ -o $@

$(ExPipeline): mypipeline.o
	$(CC) $(CFlags) $^ -o $@

%.o: %.c
	$(CC) $(CFlags) -c $< -o $@

clean:
	rm -f $(OFiles) $(Ex) $(ExPipeline)
