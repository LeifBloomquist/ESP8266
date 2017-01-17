
#define ESC  27

void AnsiClearScreen(Stream& client)
{
  // Clear ^[[2J
  client.write(ESC);
  client.write("[2J");

  // Home ^[[H
  client.write(ESC);
  client.write("[H");
}
