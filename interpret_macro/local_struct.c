#include "stdio.h"
#include "stdint.h"
#include "string.h"

unsigned char x[] = {
  0xFA, 0xCA, 0xDE, 0x50
};

void print_bytes(void* ptr, int count) {
  uint8_t* bytes = ptr;
  printf("{");
  for (int i = 0; i < count; ++i) {
    if (i + 1 == count) {
      printf("0x%x}\n", bytes[i]);
    } else {
      printf("0x%x, ", bytes[i]);
    }
  }
}

#define interpret(type_name, ptr, ...) \
    struct __attribute__((packed, may_alias)) type_name { __VA_ARGS__ } *type_name = \
    (struct type_name *)(ptr)

#define field_compare(field, x, y) \
  (sizeof(x->field) == sizeof(y->field)      \
   && memcmp(                                \
       (void*)(&(x->field)),                 \
       (void *)(&(y->field)),                \
       sizeof(x->field)))

int main() {
  void* data = &x;
  print_bytes(data, 4);
  {
    interpret(color, data, 
      uint8_t R, G, B; 
    );
    uint16_t rb;
    {
      interpret(view_rb, &rb, 
        uint8_t R, B;
      );
      *view_rb = (struct view_rb){color->R, color->B};
    }
    // rb is 0xdefa 😛
    printf("rb = 0x%x\n", rb);

    printf(
        "                         " "\n"   
        "let's revert!"             "\n"
        "                         " "\n"   
        "color(R G B)"              "\n"
        "reversed(B G R)"           "\n"
        "                         " "\n"   
        "*color = (struct color) {" "\n"
        "  reversed->R,           " "\n"
        "  reversed->G,           " "\n"
        "  reversed->B,           " "\n" 
        "};                       " "\n"   
        "                         " "\n"   
    );
    {
      interpret(reversed, data,
          uint8_t B, G, R;
      );
      *color = (struct color) {
        reversed->R,
        reversed->G,
        reversed->B,
      };
    }
  }
  print_bytes(data, 4);

  char tag[] = "__._____";
  char stream[] = "01.hello@ppmkjmoihjhuh";

  interpret(view_tag, tag, 
      struct { uint8_t _[2]; } version; 
      uint8_t _1; 
      struct { uint8_t _[5]; } id;
  );
  interpret(view_stream, stream, 
      __typeof__(view_tag->version) version;
      __typeof__(view_tag->_1) _1;
      __typeof__(view_tag->id) id;
  );

  printf("%s\n", tag);
  
  interpret(def, "05_lol_other", 
    __typeof__(view_tag->version) version;
    uint8_t _1;
    uint8_t _lol[3];
    uint8_t _2;
    __typeof__(view_tag->id) id;
  );

  if (!field_compare(version, def, view_stream)) {
    view_tag->version = view_stream->version;
    view_tag->id = view_stream->id;
  } else {
    view_tag->version = view_stream->version;
    view_tag->id = def->id;
  }

  printf("%s\n", tag);
  return 0;
}
