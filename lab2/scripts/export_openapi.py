import sys
from pathlib import Path

import yaml

ROOT_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT_DIR))

from app.database import init_db
from app.main import app


def main():
    init_db()
    with open(ROOT_DIR / 'openapi.yaml', 'w', encoding='utf-8') as f:
        yaml.safe_dump(app.openapi(), f, sort_keys=False, allow_unicode=True)


if __name__ == '__main__':
    main()
