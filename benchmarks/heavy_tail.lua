math.randomseed(os.time())
request = function()
    local r = math.random()
    if r < 0.90 then
        return wrk.format("GET", "/test/small.bin")
    else
        return wrk.format("GET", "/test/large.bin")
    end
end
