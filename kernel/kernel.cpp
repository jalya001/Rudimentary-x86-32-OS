#include "drivers/serial.hpp"
#include "drivers/vga.hpp"

extern "C" void kernel_main();

char msg[] = "Hello";
char strbuf[] = {'H', 'O', 'O', 'T'};
void kernel_main() {
  init_serial();
  init_vga();

  vga_write("hello world\n");

  serial_print('A');
  serial_print('B');
  serial_print('C');
  serial_print(msg);
  serial_print(msg);
  serial_print(124214214);
  serial_print(strbuf,4);
  serial_print("Hello World 1.\n");
  serial_print("Hello World 2.\n");
  serial_print("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? At vero eos et accusamus et iusto odio dignissimos ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti quos dolores et quas molestias excepturi sint occaecati cupiditate non provident, similique sunt in culpa qui officia deserunt mollitia animi, id est laborum et dolorum fuga. Et harum quidem rerum facilis est et expedita distinctio. Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo minus id quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis dolor repellendus. Temporibus autem quibusdam et aut officiis debitis aut rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus asperiores repellat.\n");
  
  
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('K');
  vga_put_char('\n');
  vga_write("hello world 1\n");
  vga_write("hello world 2\n");
  vga_write("hello world 3\n");
  vga_write("hello world 4\n");
  vga_write("hello world 5\n");
  vga_write("hello world 6\n");
  vga_write("hello world 7\n");
  vga_write("hello world 8\n");
  vga_scroll(3);
  vga_write("hello world 9\n");
  vga_write("hello world 10\n");
  vga_write("hello world 11\n");
  
  serial_print("finit.\n");
  

  while (1) {
      asm volatile ("hlt");
  }
}
