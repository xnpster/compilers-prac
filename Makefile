BIN = run

CXX = g++
CXXFLAGS = -O0 -g

QBE = ./qbe/

QBE_COMMOBJ  = util.o parse.o abi.o cfg.o mem.o ssa.o alias.o load.o \
           copy.o fold.o simpl.o live.o spill.o rega.o emit.o
AMD64OBJ = amd64/targ.o amd64/sysv.o amd64/isel.o amd64/emit.o
ARM64OBJ = arm64/targ.o arm64/abi.o arm64/isel.o arm64/emit.o
RV64OBJ  = rv64/targ.o rv64/abi.o rv64/isel.o rv64/emit.o
QBE_OBJ      =  $(addprefix $(QBE), $(QBE_COMMOBJ) $(AMD64OBJ) $(ARM64OBJ) $(RV64OBJ))

OBJ = main.o LoopNode/loop_node.o

.PHONY: $(BIN)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN) : $(OBJ)
	make -C $(QBE)

	$(CXX) $(CXXFLAGS) $^ $(QBE_OBJ) -o $@

clean:
	make -C $(QBE) clean

	rm -f $(OBJ) $(BIN) 