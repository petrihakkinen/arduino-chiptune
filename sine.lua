-- sine table generator

local amp = 32

for i=0,31 do
	local t = i/32 * math.pi * 2
	local v = math.floor(math.sin(t) * amp)
	v = math.max(v, -amp)
	v = math.min(v, amp-1)
	io.write(string.format("%d", v), ",")
end
io.write("\n")