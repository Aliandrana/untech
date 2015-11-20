.ifndef ::_METASPRITE__METASPRITE_H_
::_METASPRITE__METASPRITE_H_ := 1

.include "common/config.inc"
.include "common/modules.inc"
.include "common/synthetic.inc"

.include "dataformat.h"

.setcpu "65816"


METASPRITE_STATUS_PALETTE_SET_FLAG	= %10000000
METASPRITE_STATUS_DYNAMIC_TILESET_FLAG	= %01000000
METASPRITE_STATUS_VRAM_INDEX_MASK	= %00111110
METASPRITE_STATUS_VRAM_SET_FLAG		= %00000001

.struct MetaSpriteStruct
	;; Current frame being rendered/processedk
	currentFrame		.addr

	;; The state of the metasprite's VRAM/Palette allocations
	;; 	%pdiiiiiv
	;;
	;; p: palette set
	;; d: dynamic tileset active (MUST NOT be set if vram is clear)
	;; iiiii: vram slot table number (index / 2)
	;; v: vram set
	status			.byte

	;; Offset between frame object data and the OAM charattr data
	blockOneCharAttrOffset	.word
	;; Offset between frame object data and the OAM charattr data (second block)
	blockTwoCharAttrOffset	.word
.endstruct

;; The dp offset between the DP passed to the functions
;; and the the address of `MetaSprite__State`.
;;
;; Should be the location of the `MetaSprite__State` struct
;; with the entity struct.
;;
;; ::TODO make it entity::metaSpriteState in the future::
.global MetaSpriteDpOffset: zeropage

;; The FrameSet table
.global MetaSpriteFrameSetTable: far
.global MetaSpriteFrameSetTable_end: far


.importmodule MetaSprite
	;; The segment that holds the frame data.
	CONFIG	METASPRITE_ANIMATION_LIST_BLOCK, "METASPRITE_ANIMATION"
	CONFIG	METASPRITE_ANIMATION_DATA_BLOCK, "METASPRITE_ANIMATION"
	CONFIG	METASPRITE_FRAME_LIST_BLOCK, "METASPRITE_FRAME"
	CONFIG	METASPRITE_FRAME_DATA_BLOCK, "METASPRITE_FRAME"
	CONFIG	METASPRITE_FRAME_OBJECTS_BLOCK, "METASPRITE_FRAME_OBJECTS"
	CONFIG	METASPRITE_TILESET_BLOCK, "METASPRITE_TILESET_TABLE"
	CONFIG	METASPRITE_ENTITY_COLLISION_HITBOXES_BLOCK, "METASPRITE_ENTITY_COLLISION_HITBOXES"
	CONFIG	METASPRITE_TILE_COLLISION_HITBOXES_BLOCK, "METASPRITE_TILE_COLLISION_HITBOXES"
	CONFIG	METASPRITE_ACTION_POINT_BLOCK, "METASPRITE_ACTION_POINT"
	CONFIG	METASPRITE_PALETTE_LIST_BLOCK, "METASPRITE_PALETTE"
	CONFIG	METASPRITE_PALETTE_DATA_BLOCK, "METASPRITE_PALETTE"
	CONFIG	METASPRITE_DMA_TABLE_BLOCK, "METASPRITE_DMA_TABLE"

	;; MetaSprite VRAM settings
	CONFIG METASPRITE_VRAM_WORD_ADDRESS, $6000
	CONFIG_RANGE METASPRITE_DMA_TABLE_COUNT, 30, 0, 96
	CONFIG_RANGE METASPRITE_VRAM_ROW_SLOTS, 14, 0, 16
	CONFIG_RANGE METASPRITE_VRAM_TILE_SLOTS, 16, 0, 32

	;; The offset between the sprite xpos/ypos and the frame xpos/ypos
	POSITION_OFFSET = 128


	;; The offsetted position of the metasprite to render
	;; (word, value = xPos - POSITION_OFFSET)
	.importlabel xPos
	.importlabel yPos


	;; When zero, load the buffer into OAM during VBlank
	;; When buffer is copied, the variable is set to non-zero
	;; (byte, shadow)
	.importlabel updateOamBufferOnZero

	;; The buffer to load into OAM
	;; (oamBuffer_size bytes, WRAM7E)
	.importlabel oamBuffer, far
	oamBuffer_size = 128 * 4 + 128 / 4


	;; When zero, load the palette buffer into CGRAM during VBlank
	;; When buffer is copied, the variable is set to non-zero
	;; (byte, shadow)
	.importlabel updatePaletteBufferOnZero

	;; The buffer to load into CGRAM for the sprite palettes
	;; (paletteBuffer_size bytes, WRAM7E)
	.importlabel paletteBuffer, far
	paletteBuffer_size = 32 * 8



	;; Initialize the metasprite module
	;;
	;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	.importroutine Init


	;; Transfers the tile data to VRAM
	;;
	;; REQUIRES: Force-Blank
	.importroutine TransferTileBuffer

	;; Transfers data to PPU during VBlank
	;;
	;; REQUIRES: 16 bit A, 8 bit Index, DB = $80, DP = $4300, VBlank or Force Blank
	.importroutine VBlank


	;; Palettes
	;; ========

	;;; Sets the palette of a metasprite.
	;;;
	;;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	;;;
	;;; INPUT:
	;;;	DP: MetaSpriteStruct address - MetaSpriteDpOffset
	;;;	A: palette address in METASPRITE_PALETTE_DATA_BLOCK
	;;;	   points to the 15 colors in metasprite
	;;;	   MUST not be NULL
	;;;
	;;; OUTPUT: C set if succeeded
	.importroutine SetPaletteAddress

	;;; Removes the palette from the metasprite
	;;;
	;;; This function should not be called directly,
	;;; instead you should call `Deactive` ::CHECK name::
	;;;
	;;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	;;;
	;;; INPUT:
	;;;	DP: MetaSpriteStruct address - MetaSpriteDpOffset
	.importroutine RemovePalette

	;;; Retrieves the palette of a metasprite.
	;;;
	;;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	;;;
	;;; INPUT:
	;;;	DP: MetaSpriteStruct address - MetaSpriteDpOffset
	;;;
	;;; RETURN:
	;;;	A: palette address in METASPRITE_PALETTE_DATA_BLOCK
	;;;	   points to the 15 colors in metasprite
	;;;	   NULL (0) if metasprite has no palette (in which case it is removed)
	;;;	zero: set if no metasprite has no palette
	.importroutine GetPaletteAddress

	;;; Rebuilds the palette buffer
	;;;
	;;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	;;;
	.importroutine ReloadPalettes



	;; ::TODO handle rest of metasprite modules, tiles, etc::


	;; Render Loop
	;; ===========

	;;; Start a render loop
	;;;
	;;; REQUIRES: 16 bit A, DB = $7E
	.importroutine RenderLoopInit

	;;; Renders a metasprite frame
	;;;
	;;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	;;;
	;;; INPUT:
	;;;	DP: MetaSpriteStruct address - MetaSpriteDpOffset
	;;; 	xPos: sprite.xPos - POSITION_OFFSET
	;;; 	yPos: sprite.yPos - POSITION_OFFSET
	.importroutine RenderFrame

	;;; Finalizes the render loop
	;;;
	;;; REQUIRES: 16 bit A, 16 bit Index, DB = $7E
	.importroutine RenderLoopEnd

.endimportmodule

.endif

; vim: ft=asm:

