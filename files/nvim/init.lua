vim.opt.tabstop = 4        -- Number of spaces a tab counts for
vim.opt.shiftwidth = 4     -- Number of spaces for each indent
vim.opt.expandtab = true   -- Convert tabs to spaces
vim.opt.smartindent = true -- Smart indenting

-- Basic settings
vim.opt.number = true          -- Line numbers
vim.opt.relativenumber = true  -- Relative line numbers
vim.opt.termguicolors = true   -- Enable true colors

-- Packer plugin manager
local ensure_packer = function()
  local fn = vim.fn
  local install_path = fn.stdpath('data')..'/site/pack/packer/start/packer.nvim'
  if fn.empty(fn.glob(install_path)) > 0 then
    fn.system({'git', 'clone', '--depth', '1', 'https://github.com/wbthomason/packer.nvim', install_path})
    vim.cmd [[packadd packer.nvim]]
    return true
  end
  return false
end

local packer_bootstrap = ensure_packer()

require('packer').startup(function(use)
  use 'wbthomason/packer.nvim'  -- Packer itself

  -- Navigation
  use 'kyazdani42/nvim-tree.lua'
  use {
    'nvim-telescope/telescope.nvim',
    requires = { {'nvim-lua/plenary.nvim'} }
  }

  -- C++ Development
  use 'neovim/nvim-lspconfig'
  use 'L3MON4D3/LuaSnip'
  use 'hrsh7th/cmp-nvim-lsp'
  use 'hrsh7th/nvim-cmp'

  -- Bash Development
  use 'dense-analysis/ale'  -- Linting for Bash

  -- Visuals
  use 'folke/tokyonight.nvim'
  use 'nvim-lualine/lualine.nvim'
  use 'kyazdani42/nvim-web-devicons'

  -- Extras
  use 'numToStr/Comment.nvim'
  use {
    'nvim-treesitter/nvim-treesitter',
    run = ':TSUpdate'
  }

  if packer_bootstrap then
    require('packer').sync()
  end
end)

-- Plugin configurations

-- Nvim-tree (File explorer)
require('nvim-tree').setup()

-- Telescope (Fuzzy finder)
require('telescope').setup()

-- LSP for C++
local lspconfig = require('lspconfig')
lspconfig.clangd.setup{}

-- Completion setup
local cmp = require('cmp')
cmp.setup({
  snippet = {
    expand = function(args)
      require('luasnip').lsp_expand(args.body)
    end,
  },
  sources = {
    { name = 'nvim_lsp' },
  },
})

-- ALE for Bash linting
vim.g.ale_linters = {
  sh = {'shellcheck'},
}

-- Theme
vim.cmd('colorscheme tokyonight')

-- Lualine (Status bar)
require('lualine').setup()

-- Comment.nvim
require('Comment').setup()

-- Treesitter for better syntax highlighting
require('nvim-treesitter.configs').setup {
  ensure_installed = { "c", "cpp", "bash" },
  highlight = {
    enable = true,
  },
}

-- Key mappings
vim.api.nvim_set_keymap('n', '<C-n>', ':NvimTreeToggle<CR>', { noremap = true, silent = true })
vim.api.nvim_set_keymap('n', '<C-p>', ':Telescope find_files<CR>', { noremap = true, silent = true })
