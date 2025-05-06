import re
import tkinter as tk
from tkinter import ttk, filedialog, scrolledtext
from collections import defaultdict
import networkx as nx
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import seaborn as sns
from datetime import datetime
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import pandas as pd
import pickle
from typing import Dict, List, Set, Tuple
from pathlib import Path
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.decomposition import NMF
import webview
import tempfile
import os

class TopicDetector:
    def __init__(self, n_topics=10):
        self.vectorizer = TfidfVectorizer(
            max_features=5000,
            stop_words='english',
            ngram_range=(1, 3),
            token_pattern=r'(?u)\b[A-Za-z]+\b'  # Improved token pattern
        )
        self.nmf = NMF(n_components=n_topics, random_state=42)
        self.topics = []
        
    def fit(self, texts: List[str]):
        if not texts:
            return
        
        # Preprocess texts to handle potential encoding issues
        processed_texts = [self._preprocess_text(text) for text in texts]
        tfidf = self.vectorizer.fit_transform(processed_texts)
        self.nmf.fit(tfidf)
        
        feature_names = self.vectorizer.get_feature_names_out()
        self.topics = []
        for topic_idx, topic in enumerate(self.nmf.components_):
            top_words = [feature_names[i] for i in topic.argsort()[:-10-1:-1]]
            self.topics.append(top_words)
    
    def _preprocess_text(self, text: str) -> str:
        # Basic text preprocessing
        text = re.sub(r'[^\w\s]', ' ', text)
        text = re.sub(r'\s+', ' ', text)
        return text.strip().lower()
            
    def get_document_topics(self, text: str) -> Dict[int, float]:
        if not text:
            return {}
        
        text = self._preprocess_text(text)
        tfidf = self.vectorizer.transform([text])
        topic_weights = self.nmf.transform(tfidf)[0]
        return {i: float(weight) for i, weight in enumerate(topic_weights)}

class TopicVisualizer:
    def __init__(self, figure_size=(12, 8)):
        self.fig_size = figure_size
        
    def create_topic_network(self, topics: List[List[str]], 
                           topic_weights: Dict[int, float]) -> Tuple[nx.Graph, plt.Figure]:
        G = nx.Graph()
        
        # Create a more structured layout
        for topic_idx, words in enumerate(topics):
            topic_node = f"Topic {topic_idx}"
            weight = topic_weights.get(topic_idx, 0.1)  # Minimum weight to ensure visibility
            G.add_node(topic_node, node_type='topic', weight=weight)
            
            for word in words[:5]:  # Top 5 words per topic
                G.add_node(word, node_type='word')
                G.add_edge(topic_node, word, weight=weight)
        
        plt.clf()  # Clear any existing plots
        fig = plt.figure(figsize=self.fig_size)
        pos = nx.spring_layout(G, k=1.5)  # Increased spacing
        
        # Draw topic nodes
        topic_nodes = [n for n, d in G.nodes(data=True) if d['node_type'] == 'topic']
        nx.draw_networkx_nodes(G, pos, nodelist=topic_nodes, 
                             node_color='lightblue', 
                             node_size=[1500 * topic_weights.get(int(n.split()[1]), 1.0) for n in topic_nodes])
        
        # Draw word nodes
        word_nodes = [n for n, d in G.nodes(data=True) if d['node_type'] == 'word']
        nx.draw_networkx_nodes(G, pos, nodelist=word_nodes, 
                             node_color='lightgreen', 
                             node_size=800)
        
        # Draw edges with varying thickness
        edge_weights = [G[u][v]['weight'] for u, v in G.edges()]
        nx.draw_networkx_edges(G, pos, width=[w * 2 for w in edge_weights], alpha=0.6)
        
        # Add labels with better formatting
        nx.draw_networkx_labels(G, pos, font_size=8, font_weight='bold')
        
        plt.title("Topic Network Visualization", pad=20, fontsize=14)
        plt.axis('off')
        
        return G, fig

class TopicEvolutionVisualizer:
    def create_evolution_plot(self, book_topics: Dict[str, Dict[int, float]], 
                            topics: List[List[str]]) -> go.Figure:
        if not book_topics or not topics:
            return go.Figure()

        # Prepare data with chronological ordering
        data = []
        for book, topic_weights in book_topics.items():
            for topic_idx, weight in topic_weights.items():
                topic_words = ' | '.join(topics[topic_idx][:3])
                data.append({
                    'Book': book,
                    'Topic': f"T{topic_idx}: {topic_words}",
                    'Weight': weight
                })
        
        df = pd.DataFrame(data)
        
        # Create a more detailed visualization
        fig = go.Figure()
        
        # Add heatmap
        heatmap_data = df.pivot(index='Book', columns='Topic', values='Weight')
        fig.add_trace(go.Heatmap(
            z=heatmap_data.values,
            x=heatmap_data.columns,
            y=heatmap_data.index,
            colorscale='Viridis',
            showscale=True,
            colorbar=dict(title='Topic Weight')
        ))
        
        # Update layout
        fig.update_layout(
            title='Topic Evolution Across Books',
            xaxis_title='Topics',
            yaxis_title='Books',
            height=600,
            xaxis={'tickangle': 45},
            margin=dict(t=50, l=200)  # Increased margins for readability
        )
        
        return fig

class CharacterTopicVisualizer:
    def create_character_topic_plot(self, 
                                  character_topics: Dict[str, Dict[int, float]],
                                  topics: List[List[str]]) -> go.Figure:
        if not character_topics or not topics:
            return go.Figure()

        # Prepare data
        data = []
        for char, topic_weights in character_topics.items():
            for topic_idx, weight in topic_weights.items():
                topic_words = ' | '.join(topics[topic_idx][:3])
                data.append({
                    'Character': char,
                    'Topic': f"T{topic_idx}: {topic_words}",
                    'Weight': weight
                })
        
        df = pd.DataFrame(data)
        
        # Create subplots
        fig = make_subplots(
            rows=2, cols=1,
            subplot_titles=('Character-Topic Associations', 'Topic Distribution per Character'),
            heights=[0.6, 0.4],
            vertical_spacing=0.2
        )
        
        # Add heatmap
        heatmap_data = df.pivot(index='Character', columns='Topic', values='Weight')
        fig.add_trace(
            go.Heatmap(
                z=heatmap_data.values,
                x=heatmap_data.columns,
                y=heatmap_data.index,
                colorscale='Viridis',
                showscale=True,
                colorbar=dict(title='Topic Weight', y=0.8, len=0.5)
            ),
            row=1, col=1
        )
        
        # Add bar charts
        for char in df['Character'].unique():
            char_data = df[df['Character'] == char]
            fig.add_trace(
                go.Bar(
                    name=char,
                    x=char_data['Topic'],
                    y=char_data['Weight'],
                    showlegend=True
                ),
                row=2, col=1
            )
        
        # Update layout
        fig.update_layout(
            height=1000,
            title_text="Character Topic Analysis",
            xaxis2={'tickangle': 45},
            margin=dict(t=50, l=200)
        )
        
        return fig

class TopicAnalysisExporter:
    @staticmethod
    def export_analysis(filename: str,
                       topics: List[List[str]],
                       book_topics: Dict[str, Dict[int, float]],
                       character_topics: Dict[str, Dict[int, float]]):
        data = {
            'topics': topics,
            'book_topics': book_topics,
            'character_topics': character_topics,
            'timestamp': datetime.now().isoformat()
        }
        
        with open(filename, 'wb') as f:
            pickle.dump(data, f)
    
    @staticmethod
    def import_analysis(filename: str) -> Dict:
        with open(filename, 'rb') as f:
            return pickle.load(f)

class MiraCorpusAnalyzer:
    def __init__(self, root):
        self.root = root
        self.root.title("Mira Corpus Analyzer & Trainer")
        self.root.configure(bg='black')
        
        self.books: Dict[str, str] = {}
        self.book_topics: Dict[str, Dict[int, float]] = {}
        self.character_topics: Dict[str, Dict[int, float]] = {}
        self.topic_detector = TopicDetector()
        
        self.topic_visualizer = TopicVisualizer()
        self.evolution_visualizer = TopicEvolutionVisualizer()
        self.character_visualizer = CharacterTopicVisualizer()
        self.analysis_exporter = TopicAnalysisExporter()
        
        self.temp_dir = tempfile.mkdtemp()
        self.setup_gui()
        
    def setup_gui(self):
        style = ttk.Style()
        style.configure('TNotebook.Tab', padding=[10, 5])
        
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill='both', expand=True)
        
        self.books_tab = ttk.Frame(self.notebook)
        self.topics_tab = ttk.Frame(self.notebook)
        self.evolution_tab = ttk.Frame(self.notebook)
        self.characters_tab = ttk.Frame(self.notebook)
        
        self.notebook.add(self.books_tab, text='Books & Search')
        self.notebook.add(self.topics_tab, text='Topic Network')
        self.notebook.add(self.evolution_tab, text='Topic Evolution')
        self.notebook.add(self.characters_tab, text='Character Analysis')
        
        self.setup_books_tab()
        self.setup_topics_tab()
        self.setup_evolution_tab()
        self.setup_characters_tab()
        self.setup_menu()
        
    def setup_books_tab(self):
        book_frame = ttk.Frame(self.books_tab)
        book_frame.pack(fill='both', expand=True, padx=5, pady=5)
        
        # Add scrollbar
        scrollbar = ttk.Scrollbar(book_frame)
        scrollbar.pack(side='right', fill='y')
        
        self.book_tree = ttk.Treeview(book_frame, columns=('Topics',), yscrollcommand=scrollbar.set)
        self.book_tree.heading('#0', text='Book')
        self.book_tree.heading('Topics', text='Main Topics')
        self.book_tree.pack(fill='both', expand=True)
        
        scrollbar.config(command=self.book_tree.yview)
        
        search_frame = ttk.Frame(self.books_tab)
        search_frame.pack(fill='x', padx=5, pady=5)
        
        self.search_var = tk.StringVar()
        ttk.Entry(search_frame, textvariable=self.search_var).pack(side='left', fill='x', expand=True)
        ttk.Button(search_frame, text="Search", command=self.search_books).pack(side='left')
        
    def setup_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Import Books", command=self.import_books)
        file_menu.add_command(label="Export Analysis", command=self.export_analysis)
        file_menu.add_command(label="Import Analysis", command=self.import_analysis)
        
        analysis_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Analysis", menu=analysis_menu)
        analysis_menu.add_command(label="Analyze Topics", command=self.analyze_all)
        analysis_menu.add_command(label="Refresh Visualizations", command=self.refresh_visualizations)
        
    def setup_topics_tab(self):
        self.network_frame = ttk.Frame(self.topics_tab)
        self.network_frame.pack(fill='both', expand=True)
        
        controls = ttk.Frame(self.topics_tab)
        controls.pack(fill='x')
        ttk.Button(controls, text="Update Network", 
                  command=self.update_topic_network).pack(side='left')
        
    def setup_evolution_tab(self):
        self.evolution_frame = ttk.Frame(self.evolution_tab)
        self.evolution_frame.pack(fill='both', expand=True)
        
    def setup_characters_tab(self):
        self.characters_frame = ttk.Frame(self.characters_tab)
        self.characters_frame.pack(fill='both', expand=True)
        
    def import_books(self):
        files = filedialog.askopenfilenames(filetypes=[("Text Files", "*.txt")])
        for f in files:
            path = Path(f)
            try:
                with open(path, 'r', encoding='utf-8') as file:
                    content = file.read()
                    self.books[path.name] = content
                    self.book_tree.insert('', 'end', text=path.name, values=('Not analyzed',))
            except Exception as e:
                print(f"Error importing {path.name}: {e}")
                
    def analyze_topics(self):
        if not self.books:
            return
            
        self.topic_detector.fit(list(self.books.values()))
        
        for book_name, content in self.books.items():
            self.book_topics[book_name] = self.topic_detector.get_document_topics(content)
            
            top_topics = sorted(self.book_topics[book_name].items(), 
                              key=lambda x: x[1], reverse=True)[:3]
            topic_str = "; ".join([
                f"Topic {tid}: {', '.join(self.topic_detector.topics[tid][:3])}"
                for tid, _ in top_topics
            ])
            
            for item in self.book_tree.get_children():
                if self.book_tree.item(item)['text'] == book_name:
                    self.book_tree.set(item, 'Topics', topic_str)
                    
    def analyze_all(self):
        if not self.books:
            return
        self.analyze_topics()
        self.analyze_characters()
        self.refresh_visualizations()
        
    def analyze_characters(self):
        self.character_topics.clear()
        
        # Improved character dialogue extraction
        for book_name, content in self.books.items():
            character_dialogues = defaultdict(str)
            
            # More robust dialogue pattern matching
            dialogue_pattern = r'([A-Za-z][A-Za-z\s]+?):\s*([^"\'\n]+?)(?=\n|$|\s+[A-Za-z]+:)'
            for match in re.finditer(dialogue_pattern, content):
                char, dialogue = match.groups()
                char = char.strip()
                if len(char) > 1:  # Filter out single-letter characters
                    character_dialogues[char] += dialogue.strip() + " "
            
            # Process character dialogues
            for char, dialogues in character_dialogues.items():
                if len(dialogues.split()) < 10:  # Skip characters with very little dialogue
                    continue
                    
                topics = self.topic_detector.get_document_topics(dialogues)
                
                if char not in self.character_topics:
                    self.character_topics[char] = topics
                else:
                    # Merge topics with existing character topics
                    for topic_id, weight in topics.items():
                        current_weight = self.character_topics[char].get(topic_id, 0)
                        self.character_topics[char][topic_id] = max(weight, current_weight)
    
    def search_books(self):
        query = self.search_var.get().lower()
        if not query:
            return
        
        for item in self.book_tree.get_children():
            book_name = self.book_tree.item(item)['text']
            if book_name in self.books and query in self.books[book_name].lower():
                self.book_tree.selection_add(item)
            else:
                self.book_tree.selection_remove(item)
                
    def update_topic_network(self):
        if not hasattr(self, 'topic_detector') or not self.topic_detector.topics:
            return
            
        for widget in self.network_frame.winfo_children():
            widget.destroy()
            
        G, fig = self.topic_visualizer.create_topic_network(
            self.topic_detector.topics,
            {i: 1.0 for i in range(len(self.topic_detector.topics))}
        )
        
        canvas = FigureCanvasTkAgg(fig, self.network_frame)
        canvas.draw()
        canvas.get_tk_widget().pack(fill='both', expand=True)
        
    def update_evolution_plot(self):
        if not self.book_topics:
            return
            
        fig = self.evolution_visualizer.create_evolution_plot(
            self.book_topics,
            self.topic_detector.topics
        )
        
        html = fig.to_html(include_plotlyjs=True, full_html=True)
        self.display_plotly_figure(html, self.evolution_frame)
        
    def update_character_plot(self):
        if not self.character_topics:
            return
            
        fig = self.character_visualizer.create_character_topic_plot(
            self.character_topics,
            self.topic_detector.topics
        )
        
        html = fig.to_html(include_plotlyjs=True, full_html=True)
        self.display_plotly_figure(html, self.characters_frame)
        
    def display_plotly_figure(self, html: str, parent_frame: ttk.Frame):
        for widget in parent_frame.winfo_children():
            widget.destroy()
            
        # Use the temporary directory for the HTML file
        temp_file = Path(self.temp_dir) / "temp_plot.html"
        temp_file.write_text(html, encoding='utf-8')  # Explicitly use UTF-8
        
        webview.create_window('Plot', str(temp_file), parent=parent_frame)
        
    def refresh_visualizations(self):
        self.update_topic_network()
        self.update_evolution_plot()
        self.update_character_plot()
        
    def export_analysis(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".mira",
            filetypes=[("Mira Analysis", "*.mira")]
        )
        if filename:
            self.analysis_exporter.export_analysis(
                filename,
                self.topic_detector.topics,
                self.book_topics,
                self.character_topics
            )
            
    def import_analysis(self):
        filename = filedialog.askopenfilename(
            filetypes=[("Mira Analysis", "*.mira")]
        )
        if filename:
            data = self.analysis_exporter.import_analysis(filename)
            self.topic_detector.topics = data['topics']
            self.book_topics = data['book_topics']
            self.character_topics = data['character_topics']
            self.refresh_visualizations()
            
    def __del__(self):
        # Cleanup temporary files
        try:
            for file in Path(self.temp_dir).glob("*"):
                file.unlink()
            Path(self.temp_dir).rmdir()
        except:
            pass

if __name__ == "__main__":
    root = tk.Tk()
    app = MiraCorpusAnalyzer(root)
    root.mainloop()
