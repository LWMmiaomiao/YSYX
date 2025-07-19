`include "/home/cxh/ysyx-workbench/test/npc/vsrc/defines.v"
`include "/home/cxh/ysyx-workbench/test/npc/vsrc/register_flie.v"
`include "/home/cxh/ysyx-workbench/test/npc/vsrc/example/mux.v"

module PC(
  input  wire           clk,
  input  wire           rst,
  input  wire           m1,
  input  wire           m2,
  input  wire [`RegBus] result,
  input  wire [`RegBus] imm32,
  output wire [`RegBus] PCadd4,
  output reg  [`RegBus] pc
);

  wire [`RegBus] nextpc;
  wire [`RegBus] nextpc_temp;
  wire [`RegBus] PCaddIMM32;
  
  assign PCadd4     = pc + 32'd4;
  assign PCaddIMM32 = pc + imm32;

  always @(posedge clk) begin
    if(rst == 1'b1)
      pc <= 32'h8000_0000; //PC复位
    else
      pc <= nextpc;
  end

  //module MuxKeyInternal #(NR_KEY = 2, KEY_LEN = 1, DATA_LEN = 1, HAS_DEFAULT = 0) (
  //   output reg  [DATA_LEN-1 : 0]                        out,
  //   input  wire [KEY_LEN -1 : 0]                        key,
  //   input  wire [DATA_LEN - 1 :0]                       default_out,
  //   input  wire [NR_KEY * (KEY_LEN + DATA_LEN) - 1 : 0] lut
  // );

  // MUX1 module
  MuxKey #(2, 1, `BitWidth) i1(nextpc, (m1 & result[0]), {
      1'b0, nextpc_temp,
      1'b1, PCaddIMM32}
  );

  // MUX2 module
  MuxKey #(2, 1, `BitWidth) i2(nextpc_temp, m2, {
      `MUX2_PCadd4, PCadd4,
      `MUX2_result, result}
  );

endmodule


module rv32(
  input  wire           clk,
  input  wire           rst
);
  
  wire[4:0]       rs1;
  wire[4:0]       rs2;
  wire[4:0]       rd;
  wire[2:0]       funct3;
  wire[6:0]       funct7;

  wire[`RegBus]   inst;     

  wire[`RegBus]   pc;     
  wire[`TYPE_BUS] IType;      
  wire            reg_wen;    
  wire            mem_wen;    
  wire            mem_ren;    
  wire[7:0]       wmask;     
  wire[2:0]       rmask;     
  wire            m1;        
  wire            m2;        
  wire            m3;        
  wire            m4;         
  wire[1:0]       m5;        
  wire[`AlucBus]  aluc;       //alu operation type, like add, sub...
  wire[`RegBus]   PCadd4;    
  wire[`RegBus]   result;    
  wire[`RegBus]   reg_in;    
  wire[`RegBus]   src1;      
  wire[`RegBus]   src2;      
  wire[`RegBus]   imm32;    
  wire[`RegBus]   num1;       //alu operation number1       
  wire[`RegBus]   num2;       //alu operation number2
  wire[`RegBus]   mem_rdata;  

  // PC module
  PC PC_inst(
    .clk      (clk),
    .rst      (rst),
    .m1       (m1),
    .m2       (m2),
    .result   (result),
    .imm32    (imm32),
    .PCadd4   (PCadd4),
    .pc       (pc)   
  );

  // mem module
  mem mem_inst(
    .clk      (clk),  
    .mem_wen  (mem_wen),  
    .wmask    (wmask),
    .waddr    (result),
    .wdata    (src2),
    .mem_ren  (mem_ren),  
    .rmask    (rmask),
    .raddr    (result),
    .inst_addr(pc),
    .rdata    (mem_rdata),
    .inst_data(inst)
  );

  // Control Unit module
  control_unit control_unit_inst(
    .inst      (inst),
    .rd_11_7   (rd),
    .rs1_19_15 (rs1),
    .rs2_24_20 (rs2),
    .fun3_14_12(funct3),
    .fun7_31_25(funct7),
    .IType     (IType),
    .aluc      (aluc),
    .reg_wen   (reg_wen),    
    .mem_wen   (mem_wen),
    .mem_ren   (mem_ren),  
    .wmask     (wmask),
    .rmask     (rmask),
    .m1        (m1),    
    .m2        (m2),    
    .m3        (m3),   
    .m4        (m4),   
    .m5        (m5)
  );

  // Register File module
  register_file register_file_inst(
    .clk      (clk),
    .rst      (rst),
    .reg_wen  (reg_wen),
    .rs1      (rs1),
    .rs2      (rs2),
    .rd       (rd),
    .reg_in   (reg_in),
    .src1     (src1),
    .src2     (src2)
  );

  // Imm Extend module
  imm_extend imm_extend_inst(
    .rs1   (rs1),
    .rs2   (rs2),
    .rd    (rd),
    .funct3(funct3),
    .funct7(funct7),
    .IType (IType),
    .imm32 (imm32)
  );

  // MUX3 module
  MuxKey #(2, 1, `BitWidth) i3(num2, m3, {
      `MUX3_src2,  src2,
      `MUX3_imm32, imm32}
  );

  // MUX4 module
  MuxKey #(2, 1, `BitWidth) i4(num1, m4, {
      `MUX4_pc,   pc,
      `MUX4_src1, src1}
  );

  // MUX5 module
  MuxKey #(4, 2, `BitWidth) i5(reg_in, m5, {
      `MUX5_PCadd4, PCadd4,
      `MUX5_memdat, mem_rdata,
      `MUX5_result, result,
      `MUX5_IDLE,   32'hdeadbeaf}       //uae
  );
  
  // ALU module
  alu alu_inst(
    .aluc  (aluc),
    .num1  (num1),
    .num2  (num2),
    .result(result)
  );
endmodule


