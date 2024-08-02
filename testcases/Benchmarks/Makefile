CFGDIR=/home/buoffio/Documents/tesi/FreeRTOSv202112.00/FreeRTOS/stm32l152ret6-project/Tools/LLVMEmbeddedToolchainForArm-15.0.2-Linux-x86_64

preprocessing:
	cd build; clang -gline-tables-only -ffp-contract=off -target arm -march=armv4t -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-exceptions -fno-builtin -fno-rtti -emit-llvm ../simple_random.c ../$(TARGET).c -I ../include -I ../../libraries/builtinsint -DCLASS_A -S -O0 -Xclang -disable-O0-optnone; cd ..

	llvm-link build/simple_random.ll build/$(TARGET).ll -o build/$(TARGET).ll -S

	opt --enable-new-pm --passes=reg2mem,simplifycfg build/$(TARGET).ll -o build/$(TARGET).ll -S

	llvmta --fp-contract=off -mattr=-neon,+vfp4 -disable-tail-calls -float-abi=hard --ta-strict=false --ta-num-callsite-tokens=-1 --ta-num-callee-tokens=-1 --ta-num-loop-tokens=-1 --ta-analysis-entry-point=$(TARGET) --ta-muarch-type=strictlyinorder --ta-memory-type=single build/$(TARGET).ll -O0 --ta-stall-on=dramrefresh --ta-output-unknown-loops

	llvmta --fp-contract=off -mattr=-neon,+vfp4 -disable-tail-calls -float-abi=hard --ta-strict=false --ta-num-callsite-tokens=-1 --ta-num-callee-tokens=-1 --ta-num-loop-tokens=-1 --ta-analysis-entry-point=$(TARGET) --ta-muarch-type=strictlyinorder --ta-memory-type=single build/$(TARGET).ll -O0 --ta-stall-on=dramrefresh --ta-output-unknown-extfuncs


	@echo ""
	@echo ""
	@echo "Preprocessing done, please compile LoopAnnotations.csv to contiune."

all:
	llvmta --fp-contract=off -mattr=-neon,+vfp4 -disable-tail-calls -float-abi=hard --ta-strict=false --ta-num-callsite-tokens=-1 --ta-num-callee-tokens=-1 --ta-num-loop-tokens=-1 --ta-analysis-entry-point=$(TARGET) --ta-muarch-type=outoforder --ta-memory-type=separatecaches --ta-mem-latency=1 --ta-restart-after-external build/$(TARGET).ll -O0 --ta-loop-bounds-file=LoopAnnotations_$(TARGET).csv --ta-extfunc-annotation-file=ExtFuncAnnotations_$(TARGET).csv 
	#-debug-only=loopbound
