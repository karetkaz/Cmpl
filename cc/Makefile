#flags for pvmc:
#	-c <file> : compile input file
#	-e[<n>] : execute on n units
#	-o <file> : output file
#	-d : disassemble
#	-i : show information

all: ./pvmc test.pvc
	./pvmc -c test.pvc -e8 -d -i -o test.asm.txt

# must be compiled for 32 bit cpu
./pvmc: src/pvmc.c
	gcc -m32 -o ./pvmc $^ -lm

clean:
	rm -f ./pvmc test.asm.txt
