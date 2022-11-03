
#����if����ж�gccִ��ģʽ

ifeq ($(DEBUG),true)
CC = g++ -g -std=c++11
VERSION = debug
else
CC = g++ -std=c++11
VERSION = release
endif


#��������SRCS������ǰĿ¼������.c�ļ���
#wildcard��ʾɨ�赱ǰĿ¼�����е��ļ�
SRCS = $(wildcard *.cpp)

#��������OBJS��SRCS�����е� .c ��� .o
OBJS = $(SRCS:.cpp=.o)


DEPS = $(SRCS:.cpp=.d)

#ָ��bin�ļ�λ��
#BIN = /mnt/hgfs/linux/nginx
#addprefix ΪBIN����ǰ׺
BIN := $(addprefix $(BUILD_ROOT)/,$(BIN))

#����obj�ļ���ŵ�Ŀ¼��������Ŀ¼��λ��ͳһ
LINK_OBJ_DIR = $(BUILD_ROOT)/app/link_obj
DEP_DIR = $(BUILD_ROOT)/app/dep

#����objĿ¼��depĿ¼��-p��ʾ����Ѿ����ھͲ�����
$(shell mkdir -p $(LINK_OBJ_DIR))
$(shell mkdir -p $(DEP_DIR))

#:=��=����
#:=�ڽ����׶�ֱ�Ӹ�ֵ�����ַ���(��define)
#=�����н׶�ʹ�ñ�����ֵ

OBJS := $(addprefix $(LINK_OBJ_DIR)/,$(OBJS))
DEPS := $(addprefix $(DEP_DIR)/,$(DEPS))

#�ҵ�Ŀ¼�����е�.oĿ¼(����ó���)
LINK_OBJ = $(wildcard $(LINK_OBJ_DIR)/*.o)

#����������ϵʱappĿ¼��ȱ��OBJS��Ӧ�ļ�
LINK_OBJ += $(OBJS)

#�ű���ʼ�������
all:$(DEPS) $(OBJS) $(BIN)

#��������.d�ļ�
ifneq ("$(wildcard $(DEPS))","")
include $(DEPS)
endif

#������.o�ļ������������
$(BIN):$(LINK_OBJ)
	@echo "-------------------------------build $(VERSION) mode--------------------------------------------"

	$(CC) -o $@ $^ -lpthread

#������.c�ļ�����Ϊ.o
$(LINK_OBJ_DIR)/%.o:%.cpp
	$(CC) -I$(INCLUDE_PATH) -o $@ -c $(filter %.cpp,$^)

#��¼������Ϣ
$(DEP_DIR)/%.d:%.cpp
	echo -n $(LINK_OBJ_DIR)/ > $@
	$(CC) -I$(INCLUDE_PATH) -MM $^ >> $@


