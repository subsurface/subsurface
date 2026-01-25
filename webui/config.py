# SPDX-License-Identifier: GPL-2.0
# Flask configuration for Subsurface Web UI

import os
import sys


class Config:
    """Base configuration."""
    # SECRET_KEY must match the flask-auth-proxy app's SECRET_KEY
    # to enable shared session authentication between the two apps
    SECRET_KEY = os.environ.get('SECRET_KEY', 'dev-secret-key-change-in-production')

    # Path to subsurface-cli binary
    CLI_PATH = os.environ.get('SUBSURFACE_CLI_PATH', 'subsurface-cli')

    # Path to CLI config file (for single-user mode)
    CLI_CONFIG_PATH = os.environ.get('SUBSURFACE_CLI_CONFIG', None)

    # For multi-tenant mode: base directory for user repos
    REPO_BASE_PATH = os.environ.get('SUBSURFACE_REPO_BASE', None)

    # Temporary directory for CLI config files (multi-tenant mode)
    TEMP_DIR = os.environ.get('SUBSURFACE_TEMP_DIR', '/run/subsurface-webui')

    # Development mode: skip authentication
    DEV_MODE = os.environ.get('SUBSURFACE_DEV_MODE', 'false').lower() == 'true'
    DEV_USER = os.environ.get('SUBSURFACE_DEV_USER', 'dev@localhost')


class DevelopmentConfig(Config):
    """Development configuration."""
    DEBUG = True
    DEV_MODE = True


class ProductionConfig(Config):
    """Production configuration.

    Security requirements:
    - SECRET_KEY must be set via environment variable (same as flask-auth-proxy)
    - DEV_MODE is always False
    - DEBUG is always False
    """
    DEBUG = False
    DEV_MODE = False

    def __init__(self):
        super().__init__()
        # Security: Fail loudly if SECRET_KEY is not properly configured
        if self.SECRET_KEY == 'dev-secret-key-change-in-production':
            print("ERROR: SECRET_KEY environment variable must be set in production!", file=sys.stderr)
            print("It must match the SECRET_KEY from your flask-auth-proxy configuration.", file=sys.stderr)
            sys.exit(1)
