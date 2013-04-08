-- generate oscillator frequencies

freq = 440
r = 1.059454545
sampleFreq = 16000

cnt = 0

for i=-57,38 do
	local f = freq * r^i
	--print(string.format("%.1f", f))
	f = f * 65536 / sampleFreq
	io.write(string.format("0x%04x", f), ",")
	cnt = cnt + 1
	if cnt == 12 then
		io.write("\n")
		cnt = 0
	end
end
