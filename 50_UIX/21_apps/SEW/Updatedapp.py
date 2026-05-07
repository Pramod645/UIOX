#— Flask Web UI

from flask import Flask, request, render_template
from engine import SearchEngine

app = Flask(__name__)

# ── Uncomment to crawl a real site ───────────────────────────────
# engine = SearchEngine(seed_url="[en.wikipedia.org](https://en.wikipedia.org/wiki/Python_(programming_language))",
#                       max_pages=40)

# ── Demo mode (no crawl needed) ──────────────────────────────────
engine = SearchEngine()


@app.route("/", methods=["GET", "POST"])
def index():
    result_data = None
    query       = ""

    if request.method == "POST":
        query       = request.form.get("query", "").strip()
        if query:
            result_data = engine.search(query)

    return render_template("index.html",
                           data=result_data,
                           query=query)


@app.route("/reindex", methods=["POST"])
def reindex():
    """Force a re-crawl and re-index."""
    global engine
    seed = request.form.get("seed_url", "").strip()
    engine = SearchEngine(
        seed_url=seed or None,
        max_pages=int(request.form.get("max_pages", 30)),
        force_recrawl=True
    )
    return {"status": "ok", "message": "Re-indexed successfully."}


if __name__ == "__main__":
    app.run(debug=True)
