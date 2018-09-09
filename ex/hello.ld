SECTIONS
{
  /* location counter */
  . = 0x08048000 + SIZEOF_HEADERS;
  .text : { *(.text) }
  PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
  .rodata : { *(.rodata) }

	PROVIDE(data = .);
  .data : { *(.data) }
	/* Adjust the address for the data segment to the next page */
	/*. = ALIGN(0x1000); */
	PROVIDE(edata = .);
  .bss : { *(.bss) }
	PROVIDE(end = .);

	/DISCARD/ : {
		*(.eh_frame_hdr .note.gnu.build-id)
	}
}
