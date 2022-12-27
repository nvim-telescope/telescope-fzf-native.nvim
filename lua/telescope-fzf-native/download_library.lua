local uv = vim.loop
local plugin_path = string.sub(debug.getinfo(1).source, 2, #"/lua/telescope-fzf-native/download_library.lua" * -1)
local releases_url = "https://github.com/nvim-telescope/telescope-fzf-native.nvim/releases/download"

local get_platform = function()
    if vim.fn.has("win32") == 1 then
        return 'windows'
    end

    if vim.fn.has("mac") then
        return 'macos'
    end

    return "ubuntu"
end

local get_valid_compiler = function(platform)
    if platform == 'windows' then
        return 'cc'
    elseif platform == 'macos' then
        return 'gcc'
    elseif platform == 'linux' then
        return 'gcc'
    end
end

local path_join = function(strings)
    local path_separator = (platform == "windows") and '\\' or '/'

    return table.concat(strings, path_separator)
end

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

    local buffer = ""

    local real_cmd = table.remove(cmd, 1)

    uv.spawn(real_cmd, {
        args = cmd,
        stdio = { nil, stdout },
        verbatim = true
    }, function()
        stdout:read_stop()
        if (type(on_exit) == "function") then
            on_exit(buffer)
        end
    end)

    uv.read_start(stdout, function(err, data)
        assert(not err, err)
        if data then
            buffer = buffer .. data
        end
    end)

end

local download = function(options)
    options = options or {}
    local platform = options.platform or get_platform()
    local compiler = options.compiler or get_valid_compiler(platform)
    local version = options.version or "latest"

    local command = nil

    if platform == 'windows' then
        command = {
            'curl', '-L',
            string.format("%s/%s/windows-2019-%s-libfzf.dll", releases_url, version, compiler),
            '-o', path_join({ plugin_path, 'build', 'libfzf.dll' })
        }
    end

    if platform == 'ubuntu' then
        command = {
            "curl", "-L",
            string.format("%s/%s/ubuntu-%s-libfzf.so", releases_url, version, compiler),
            "-o", path_join({ plugin_path, 'build', 'libfzf.so' })
        }
    end

    if platform == 'macos' then
        command = {
            "curl", "-L",
            string.format("%s/%s/macos-%s-libfzf.so", releases_url, version, compiler),
            "-o", path_join({ plugin_path, 'build', 'libfzf.so' })
        }
    end

    --
    -- Ensure the Build directory exists
    --
    -- using format, becase we need to run the command in a subshell on windows.
    --
    spawn({
        'sh',
        '-c',
        string.format("' mkdir %s'", path_join({ plugin_path, 'build' }))
    })

    --
    -- Curl the download
    --
    spawn(command)

end

return download
