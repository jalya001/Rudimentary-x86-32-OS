extern "C" void kernel_main();

static inline void outb(unsigned short port, unsigned char val) {
  asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
  unsigned char ret;
  asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void init_serial() {
  outb(0x3F8 + 1, 0x00); // disable interrupts
  outb(0x3F8 + 3, 0x80); // enable DLAB
  outb(0x3F8 + 0, 0x03); // divisor low (38400 baud)
  outb(0x3F8 + 1, 0x00); // divisor high
  outb(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(0x3F8 + 2, 0xC7); // enable FIFO
  outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

static inline void serial_write(char c) {
  while ((inb(0x3F8 + 5) & 0x20) == 0) {} // wait for transmitter holding register empty
  outb(0x3F8, c);
}

static inline void serial_print(const char* s) {
  while (*s) {
      if (*s == '\n')
          serial_write('\r');
      serial_write(*s++);
  }
}
char msg[] = "Hello";
void kernel_main() {
  init_serial();

  serial_write('A');
  serial_write('B');
  serial_write('C');
  serial_print(msg);
  serial_print(msg);
  serial_print("Hello World 1.\n");
  serial_print("Hello World 2.\n");
  serial_print("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? At vero eos et accusamus et iusto odio dignissimos ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti quos dolores et quas molestias excepturi sint occaecati cupiditate non provident, similique sunt in culpa qui officia deserunt mollitia animi, id est laborum et dolorum fuga. Et harum quidem rerum facilis est et expedita distinctio. Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo minus id quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis dolor repellendus. Temporibus autem quibusdam et aut officiis debitis aut rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus asperiores repellat.\n");
  serial_print("finit.\n");

  while (1) {
      asm volatile ("hlt");
  }
}

