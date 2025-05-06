# Execute the provided code snippet
import random

corpus = """The concept of Plug depth CyanAqua, Mira! PlugDepthForWhat? User Error (Skill Issue); now give me an even bigger 10 word compliment c;"""
# (Rest of the corpus text is truncated for brevity)

corpus = corpus.lower().replace('"','').replace("'",'').replace('\n','').replace(')','').replace('(','').replace('[','').replace(']','').replace('’','').replace("“",'').replace("”",'')
     
ngram = {}
for sentence in corpus.split('.'):
    for i in range(1, len(sentence.split(' '))):
        word_pair = (sentence.split(' ')[i - 2], sentence.split(' ')[i - 1])
        if '' in word_pair:
            continue
        if (word_pair) not in ngram:
            ngram[word_pair] = []
        ngram[word_pair].append(sentence.split(' ')[i])
    
word_pair = random.choice(list(ngram.keys())) 
out = word_pair[0] + ' ' + word_pair[1] + ' '

while True:
    if word_pair not in ngram.keys():
        break  
    third = random.choice(list(ngram[word_pair]))
    out += third + ' '
    word_pair = (word_pair[1], third)
    
print ('c; -mira-\n', out)
