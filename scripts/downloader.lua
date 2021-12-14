local uv = vim.loop

local write_async = function(path, txt, flag)
  uv.fs_open(path, flag, 438, function(open_err, fd)
    assert(not open_err, open_err)
    uv.fs_write(fd, txt, -1, function(write_err)
      assert(not write_err, write_err)
      uv.fs_close(fd, function(close_err)
        assert(not close_err, close_err)
      end)
    end)
  end)
end

local spawn = function(cmd, on_exit)
  local stdout = uv.new_pipe()
  local out = ""

  local real_cmd = table.remove(cmd, 1)
  uv.spawn(real_cmd, {
    args = cmd,
    stdio = { nil, stdout },
  }, function()
    stdout:read_stop()
    on_exit(out)
  end)

  uv.read_start(stdout, function(err, data)
    assert(not err, err)
    if data then
      out = out .. data
    end
  end)
end

spawn({ "gh", "api", "repos/nvim-telescope/telescope-fzf-native.nvim/actions/artifacts" }, function(json)
  local mes = vim.json.decode(json)
  local id = mes.artifacts[1].id
  spawn(
    { "gh", "api", "repos/nvim-telescope/telescope-fzf-native.nvim/actions/artifacts/" .. id .. "/zip" },
    function(zip)
      write_async("libfzf.zip", zip, "w")
    end
  )
end)
