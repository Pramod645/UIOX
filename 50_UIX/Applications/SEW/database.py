#— SQLite Persistence
import sqlite3
import json
import os

DB_PATH = "search_engine.db"


def get_conn():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    with get_conn() as conn:
        conn.executescript("""
            CREATE TABLE IF NOT EXISTS documents (
                url     TEXT PRIMARY KEY,
                title   TEXT,
                text    TEXT,
                links   TEXT    -- JSON list of outbound links
            );

            CREATE TABLE IF NOT EXISTS inverted_index (
                term    TEXT,
                url     TEXT,
                tf      REAL,
                PRIMARY KEY (term, url)
            );

            CREATE TABLE IF NOT EXISTS doc_freq (
                term    TEXT PRIMARY KEY,
                df      INTEGER
            );

            CREATE TABLE IF NOT EXISTS pagerank (
                url     TEXT PRIMARY KEY,
                score   REAL
            );

            CREATE TABLE IF NOT EXISTS metadata (
                key     TEXT PRIMARY KEY,
                value   TEXT
            );
        """)
    print("[DB] Initialized database.")


def save_pages(pages: dict):
    """Persist crawled pages."""
    with get_conn() as conn:
        for url, data in pages.items():
            conn.execute("""
                INSERT OR REPLACE INTO documents (url, title, text, links)
                VALUES (?, ?, ?, ?)
            """, (
                url,
                data.get("title", ""),
                data.get("text",  ""),
                json.dumps(data.get("links", []))
            ))
    print(f"[DB] Saved {len(pages)} documents.")


def load_pages() -> dict:
    """Load all crawled pages from DB."""
    with get_conn() as conn:
        rows = conn.execute(
            "SELECT url, title, text, links FROM documents"
        ).fetchall()
    pages = {}
    for row in rows:
        pages[row["url"]] = {
            "title": row["title"],
            "text" : row["text"],
            "links": json.loads(row["links"] or "[]")
        }
    return pages


def save_index(inverted_index: dict, doc_freq: dict):
    """Persist inverted index and document frequencies."""
    with get_conn() as conn:
        conn.execute("DELETE FROM inverted_index")
        conn.execute("DELETE FROM doc_freq")

        for term, url_tf in inverted_index.items():
            for url, tf in url_tf.items():
                conn.execute("""
                    INSERT OR REPLACE INTO inverted_index (term, url, tf)
                    VALUES (?, ?, ?)
                """, (term, url, tf))

        for term, df in doc_freq.items():
            conn.execute("""
                INSERT OR REPLACE INTO doc_freq (term, df)
                VALUES (?, ?)
            """, (term, df))

    print("[DB] Saved inverted index.")


def load_index():
    """Load inverted index and doc freq from DB."""
    from collections import defaultdict
    inverted_index = defaultdict(dict)
    doc_freq       = defaultdict(int)

    with get_conn() as conn:
        for row in conn.execute("SELECT term, url, tf FROM inverted_index"):
            inverted_index[row["term"]][row["url"]] = row["tf"]
        for row in conn.execute("SELECT term, df FROM doc_freq"):
            doc_freq[row["term"]] = row["df"]

    return inverted_index, doc_freq


def save_pagerank(scores: dict):
    """Persist PageRank scores."""
    with get_conn() as conn:
        conn.execute("DELETE FROM pagerank")
        for url, score in scores.items():
            conn.execute("""
                INSERT OR REPLACE INTO pagerank (url, score)
                VALUES (?, ?)
            """, (url, score))
    print("[DB] Saved PageRank scores.")


def load_pagerank() -> dict:
    """Load PageRank scores from DB."""
    with get_conn() as conn:
        rows = conn.execute("SELECT url, score FROM pagerank").fetchall()
    return {row["url"]: row["score"] for row in rows}


def is_indexed() -> bool:
    """Check if index already exists in DB."""
    with get_conn() as conn:
        count = conn.execute(
            "SELECT COUNT(*) FROM inverted_index"
        ).fetchone()[0]
    return count > 0


def set_meta(key, value):
    with get_conn() as conn:
        conn.execute("""
            INSERT OR REPLACE INTO metadata (key, value) VALUES (?, ?)
        """, (key, str(value)))


def get_meta(key):
    with get_conn() as conn:
        row = conn.execute(
            "SELECT value FROM metadata WHERE key=?", (key,)
        ).fetchone()
    return row["value"] if row else None
