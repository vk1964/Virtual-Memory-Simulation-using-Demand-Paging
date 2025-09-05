run:
	gcc master.c -o master -lm
	gcc shed.c -o sheduler
	gcc mmu.c -o mmu -lm
	gcc process.c -o process
	./master

clean:
	rm -f master sheduler mmu process result.txt
