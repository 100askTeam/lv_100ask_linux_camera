CAMERA_100ASK 	   ?= camera_100ask
VIDEO2LCD_DIR_NAME ?= $(CAMERA_100ASK)/video2lcd

override CFLAGS := -I$(LVGL_DIR)/$(CAMERA_100ASK) -I$(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/include -I$(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/convert $(CFLAGS)

CSRCS += $(wildcard $(LVGL_DIR)/$(CAMERA_100ASK)/assets/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(CAMERA_100ASK)/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/convert/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/display/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/gtkdrv/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/render/operation/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/$(VIDEO2LCD_DIR_NAME)/video/*.c)
