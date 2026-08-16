    .section .text.stock_disc,"ax",@progbits
    .align 2

    .global original_bs2tick
    .type original_bs2tick, @function
original_bs2tick:
    // Replay the instruction replaced by patch.s, then continue at BS2Tick + 4.
    mflr %r0
    lis %r12, stock_bs2tick_resume@ha
    lwz %r12, stock_bs2tick_resume@l(%r12)
    mtctr %r12
    bctr
    .size original_bs2tick, .-original_bs2tick

    .global gameselect_draw_dispatch
    .type gameselect_draw_dispatch, @function
gameselect_draw_dispatch:
    lis %r12, stock_disc_mode@ha
    lwz %r12, stock_disc_mode@l(%r12)
    cmpwi %r12, 0
    beq .Lcustom_draw

    // Replay the replaced instruction and resume the original Game Play renderer.
    addi %r3, %r1, 0x74
    lis %r12, stock_gameselect_draw_resume@ha
    lwz %r12, stock_gameselect_draw_resume@l(%r12)
    mtctr %r12
    bctr

.Lcustom_draw:
    // Preserve Cubiboot's original behavior: retain the matrices produced by the stock
    // setup code, then skip its widgets so only the custom browser is drawn.
    addi %r3, %r1, 0x74
    addi %r4, %r1, 0x14
    bl set_gameselect_view
    lis %r12, stock_gameselect_draw_skip@ha
    lwz %r12, stock_gameselect_draw_skip@l(%r12)
    mtctr %r12
    bctr
    .size gameselect_draw_dispatch, .-gameselect_draw_dispatch
