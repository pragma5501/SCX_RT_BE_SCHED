include make_scripts/Build.include
include make_scripts/Makefile.arch
include make_scripts/Makefile.include

ARCH ?= $(shell uname -m | sed 's/x86_64/x86/' \
			 | sed 's/arm.*/arm/' \
			 | sed 's/aarch64/arm64/' \
			 | sed 's/ppc64le/powerpc/' \
			 | sed 's/mips.*/mips/' \
			 | sed 's/riscv64/riscv/' \
			 | sed 's/loongarch64/loongarch/')

ifneq ($(LLVM),)
ifneq ($(filter %/,$(LLVM)),)
LLVM_PREFIX := $(LLVM)
else ifneq ($(filter -%,$(LLVM)),)
LLVM_SUFFIX := $(LLVM)
endif

CLANG_TARGET_FLAGS_arm          := arm-linux-gnueabi
CLANG_TARGET_FLAGS_arm64        := aarch64-linux-gnu
CLANG_TARGET_FLAGS_hexagon      := hexagon-linux-musl
CLANG_TARGET_FLAGS_m68k         := m68k-linux-gnu
CLANG_TARGET_FLAGS_mips         := mipsel-linux-gnu
CLANG_TARGET_FLAGS_powerpc      := powerpc64le-linux-gnu
CLANG_TARGET_FLAGS_riscv        := riscv64-linux-gnu
CLANG_TARGET_FLAGS_s390         := s390x-linux-gnu
CLANG_TARGET_FLAGS_x86          := x86_64-linux-gnu
CLANG_TARGET_FLAGS              := $(CLANG_TARGET_FLAGS_$(ARCH))

CLANG_FLAGS     += --target=$(CLANG_TARGET_FLAGS)
CC := clang $(CLANG_FLAGS) -fintegrated-as
endif



KERNEL_HEADERS := /lib/modules/$(shell uname -r)/build
TOOLSDIR 	:= $(KERNEL_SRC)/tools
LIBDIR 		:= $(TOOLSDIR)/lib
BPFDIR		:= $(LIBDIR)/bpf
TOOLSINCDIR	:= $(TOOLSDIR)/include
APIDIR		:= $(TOOLSINCDIR)/uapi


OBJ_DIR = ./obj
BIN_DIR = ./bin
SRC_DIR = ./src

SKEL_DIR = ./skeleton
INC_DIR = ./include

SCX_INC_DIR = $(INC_DIR)/scx

BPF_SRC_DIR = $(SRC_DIR)/ebpf
BPF_OBJ_DIR = $(OBJ_DIR)/ebpf

USR_SRC_DIR = $(SRC_DIR)/usr
USR_OBJ_DIR = $(OBJ_DIR)/usr

TEST_SRC_DIR = $(SRC_DIR)/test
TEST_BIN_DIR = $(BIN_DIR)/test

CFLAGS_INCLUDE += \
	-I $(INC_DIR) \
	-I $(SKEL_DIR) \
	-I /usr/include \
	-I /usr/include/bpf \

	
BPF_SRC += \
	$(BPF_SRC_DIR)/tracepoint.bpf.c \

USER_APP_SRC += \
	$(USR_SRC_DIR)/attach.c \
	$(USR_SRC_DIR)/monitor.c \
	
TEST_APP_SRC += \
	$(TEST_SRC_DIR)/test_sched_setattr.c \


CFLAGS += \
	-g \
	-O2 \
	-target bpf \
	-mllvm \
	-bpf-stack-size=1024 \
	-D__TARGET_ARCH_$(ARCH) \
	$(CFLAGS_INCLUDE) \

USR_CFLAGS += \
	-g \
	-O2 \
	-D_GNU_SOURCE \
	$(CFLAGS_INCLUDE)

V = 0
ifeq ($(V),1)
	Q =
else
	Q = @
endif

G_S = \033[0;32m
G_L = \033[0m

RUNFILE = run_rtsched



TARGET_BPF_OBJ = \
    $(patsubst $(BPF_SRC_DIR)/%.bpf.c, $(BPF_OBJ_DIR)/%.bpf.o, $(BPF_SRC))

TARGET_BPF_SKEL = \
	$(patsubst $(BPF_OBJ_DIR)/%.bpf.o, $(SKEL_DIR)/%.skel.h, $(TARGET_BPF_OBJ))

TARGET_USER_APP = \
	$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(USER_APP_SRC))

TARGET_TEST_APP =\
	$(patsubst $(TEST_SRC_DIR)/%.c, $(TEST_BIN_DIR)/%, $(TEST_APP_SRC))

all: runfile test

runfile: debug make_setting $(TARGET_BPF_OBJ) $(TARGET_BPF_SKEL) $(TARGET_USER_APP)
	$(Q)clang -o $(RUNFILE) $(CFLAGS_INCLUDE) $(TARGET_USER_APP) -lbpf -lelf 
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ] : Build $(RUNFILE) )

# Build BPF code
$(BPF_OBJ_DIR)/%.bpf.o: $(BPF_SRC_DIR)/%.bpf.c
	$(Q)clang $(CFLAGS_INCLUDE) $(CFLAGS) \
		-c $< -o $@
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ] : Compile $< -> $@)

# Generate BPF skeletons
$(SKEL_DIR)/%.skel.h: $(BPF_OBJ_DIR)/%.bpf.o
	$(Q)bpftool gen skeleton $< > $@
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ] : Generate $@)

$(USR_OBJ_DIR)/%.o: $(USR_SRC_DIR)/%.c
	$(Q)clang $(USR_CFLAGS) -c $< -o $@  $(CFLAGS_INCLUDE)
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ] : Compile $< -> $@ )

test: $(TARGET_TEST_APP)

# For Test : Generate Test file object
$(TEST_BIN_DIR)/%: $(TEST_SRC_DIR)/%.c
	$(Q)clang $(USR_CFLAGS) $< -o $@ -lbpf -lelf 
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ] : Compile $< -> $@ )

	

debug:       
	$(info )
	$(info -----[DEBUG]-----)
	$(info BPF_SRC_DIR = $(BPF_SRC_DIR))
	$(info TARGET_BPF_OBJ = $(TARGET_BPF_OBJ))
	$(info TARGET_BPF_SKEL = $(TARGET_BPF_SKEL))
	$(info TARGET_USER_APP = $(TARGET_USER_APP))
	$(info TARGET_TEST_APP = $(TARGET_TEST_APP))
	$(info -----[DEBUG DONE]----- )

make_setting:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BPF_OBJ_DIR)
	@mkdir -p $(USR_OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(TEST_BIN_DIR)
	@mkdir -p $(SKEL_DIR)
	@bpftool btf dump file /sys/kernel/btf/vmlinux format c > $(INC_DIR)/vmlinux.h
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ] : Make Directory )


clean:
	@rm -rf $(BIN_DIR)
	@rm -rf $(OBJ_DIR)
	@rm -rf $(SKEL_DIR)
	@rm -f $(RUNFILE)
	$(info [ $(shell echo "$(G_S)OK$(G_L)") ]: Clean)

.PHONY: tests
tests: $(TARGET_TEST_APP)