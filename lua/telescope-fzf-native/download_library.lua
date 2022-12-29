local uv = vim.loop
local plugin_path = string.sub(debug.getinfo(1).source, 2, #"//lua/telescope-fzf-native/download_library.lua" * -1)
local releases_url = "https://github.com/airtonix/telescope-fzf-native.nvim/releases/download"

print(debug.getinfo(1).source)

local get_platform = function()
    if vim.fn.has("win32") == 1 then
        return 'windows'
    end

    if vim.fn.has("mac") == 1 then
        return 'macos'
    end

    return "ubuntu"
end

local get_valid_compiler = function(platform)
    if platform == 'windows' then
        return 'cc'
    elseif platform == 'macos' then
        return 'gcc'
    elseif platform == 'ubuntu' then
        return 'gcc'
    end
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

--
-- Given a platform, compiler and version; download
-- the appropriate binary from our releases on github.
--
-- Ensures the expected 'build' directory exists before
-- downloading.
--
-- Reminder:
--
-- The filenames to download are defined as a side effect of the
-- matrix build names in `.github/actions/compile-unix/action.yml` and
-- `.github/actions/compile-windows/action.yml`, then also due to how
-- `.github/actions/prepare-artifacts/action.yml` downloads the artifacts
-- before uploading as releases.
--
-- The filename pattern is generally:
--
--   mac and linux: {github-runner-osname}-{compiler}/libfzf.so
--   windows: {github-runner-osname}-{compiler}/libfzf.dll
--
-- Files in a github release are required to be unique per filename, so we
-- mutate the above downloaded artifacts into:
--
--   {github-runner-osname}-{compiler}-libfzf.{type}
--
-- This downloader then can pick the right binary and save it the users
-- local disk as `libfzf.so` or `libfzf.dll`
--
--
return function(options)
    options = options or {}
    local platform = options.platform or get_platform()
    local compiler = options.compiler or get_valid_compiler(platform)
    local version = options.version or "dev"

    local path_separator = (platform == "windows") and '\\' or '/'
    local build_path = table.concat({ plugin_path, 'build' }, path_separator)

    local download_file, binary_file = (function()
        if platform == 'windows' then
            return string.format("windows-2019-%s-libfzf.dll", compiler),
                'libfzf.dll'
        end

        if platform == 'ubuntu' then
            return string.format("ubuntu-%s-libfzf.so", compiler),
                'libfzf.so'
        end

        if platform == 'macos' then
            return string.format("macos-%s-libfzf.so", compiler),
                'libfzf.so'
        end
    end)()

    print('creating build dir', build_path)

    spawn({
        -- we need to run the command in a subshell on windows.
        'sh', '-c',
        -- Unsure if the space here before mkdir is required for windows.
        string.format("mkdir %s", build_path)
    })

    local download_url = table.concat({ releases_url, version, download_file }, path_separator)
    local output_path = table.concat({ build_path, binary_file }, path_separator)

    print('downloading', download_url, 'to', output_path)

    spawn({
        'curl',
        '-L', download_url,
        '-o', output_path
    })

    print('downloaded')
end
