`include "/home/cxh/ysyx-workbench/test/npc/vsrc/defines.v"

module register_file(
    input  wire           clk,
    input  wire           rst,
    input  wire           reg_wen,
    input  wire [4:0]     rs1,
    input  wire [4:0]     rs2,
    input  wire [4:0]     rd,
    input  wire [`RegBus] reg_in,
    output wire [`RegBus] src1,
    output wire [`RegBus] src2
);

    integer i;
    reg[`RegBus] regs[`BitWidth-1 : 0];


    //wire register
    always @(posedge clk) begin
        if(rst == 1'b1) begin
            for(i=0; i<`RegNum; i=i+1) begin
                regs[i] <= 32'd0;  
            end
        end else if(reg_wen == 1'b1)
            regs[rd] <= reg_in; 
    end

    //read register
    assign src1 = (rs1 == 5'd0) ? 32'd0 : regs[rs1];
    assign src2 = (rs2 == 5'd0) ? 32'd0 : regs[rs2];
   
endmodule
