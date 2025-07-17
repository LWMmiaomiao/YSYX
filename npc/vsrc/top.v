module top(
  input a,
  input b,
  output f
);
  assign f = a ^ b; // 双控开关逻辑
endmodule