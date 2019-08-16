set target-async on
set confirm off
set history save
set mem inaccessible-by-default off
set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4
file kicad-workshop.elf
target remote localhost:3333
#target remote | openocd -f openocd.cfg -c "gdb_port pipe; log_output openocd.log"
