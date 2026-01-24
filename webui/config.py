# SPDX-License-Identifier: GPL-2.0
# Flask configuration for Subsurface Web UI

import os

class Config:
    """Base configuration."""
    SECRET_KEY = os.environ.get('SECRET_KEY', 'dev-secret-key-change-in-production')

    # Path to subsurface-cli binary
    CLI_PATH = os.environ.get('SUBSURFACE_CLI_PATH', 'subsurface-cli')

    # Path to CLI config file (for single-user mode)
    CLI_CONFIG_PATH = os.environ.get('SUBSURFACE_CLI_CONFIG', None)

    # For multi-tenant mode: base directory for user repos
    REPO_BASE_PATH = os.environ.get('SUBSURFACE_REPO_BASE', None)

    # Temporary directory for CLI config files (multi-tenant mode)
    TEMP_DIR = os.environ.get('SUBSURFACE_TEMP_DIR', '/tmp/subsurface-webui')

    # Header name for authenticated user (from Apache/proxy)
    AUTH_HEADER = os.environ.get('SUBSURFACE_AUTH_HEADER', 'X-Authenticated-User')

    # Development mode: skip authentication
    DEV_MODE = os.environ.get('SUBSURFACE_DEV_MODE', 'false').lower() == 'true'
    DEV_USER = os.environ.get('SUBSURFACE_DEV_USER', 'dev@localhost')


class DevelopmentConfig(Config):
    """Development configuration."""
    DEBUG = True
    DEV_MODE = True


class ProductionConfig(Config):
    """Production configuration."""
    DEBUG = False
    DEV_MODE = False
