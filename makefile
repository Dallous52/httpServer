include config.mk

all:

	#�����ر���ÿһ����Ҫ������ļ���
	@for dir in $(BUILD_DIR);\
	do\
		make -C $$dir;\
	done


clean:
	
	rm -rf app/link_obj app/dep nginx
	rm -rf signal/*.ghc app/*.ghc	
	rm -f misc/log
	rm -f misc/httpinfo
