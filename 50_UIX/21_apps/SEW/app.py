#— Flask Web UI
from flask import Flask, request, render_template
from engine import SearchEngine

app = Flask(__name__)

# ── Seed with a real site or supply local pages ──────────────────
# engine = SearchEngine(seed_url="[example.com](https://example.com)", max_pages=30)

# For quick demo without crawling:
DEMO_PAGES = {
    "[example.com](https://example.com/python)": {
        "title"  : "Python Programming Language",
        "text"   : "Python is a high-level general-purpose programming language. "
                   "It supports multiple paradigms including procedural, object-oriented, "
                   "and functional programming. Python is widely used in data science, "
                   "machine learning, web development, and automation."
    },
    "[example.com](https://example.com/linux)": {
        "title"  : "Linux Kernel",
        "text"   : "Linux is a free and open-source Unix-like operating system kernel. "
                   "It was created by Linus Torvalds in 1991. Linux powers servers, "
                   "embedded systems, Android smartphones, and supercomputers worldwide."
    },
    "[example.com](https://example.com/ml)": {
        "title"  : "Machine Learning",
        "text"   : "Machine learning is a subset of artificial intelligence that enables "
                   "systems to learn from data. Common algorithms include decision trees, "
                   "neural networks, support vector machines, and k-nearest neighbors."
    },
    "[example.com](https://example.com/databases)": {
        "title"  : "Database Systems",
        "text"   : "A database is an organized collection of structured data. "
                   "Relational databases use SQL for querying. Popular systems include "
                   "PostgreSQL, MySQL, SQLite, and Oracle. NoSQL databases like MongoDB "
                   "store unstructured or semi-structured data."
    },
}

engine = SearchEngine(pages=DEMO_PAGES)


@app.route("/", methods=["GET", "POST"])
def index():
    results = []
    query   = ""
    if request.method == "POST":
        query   = request.form.get("query", "")
        results = engine.search(query)
    return render_template("index.html", results=results, query=query)


if __name__ == "__main__":
    app.run(debug=True)
