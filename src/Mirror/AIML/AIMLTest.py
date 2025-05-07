import aiml
import os

# Create the kernel (the "brain" of the bot)
kernel = aiml.Kernel()

# Check if a brain file exists to speed up load time
if os.path.isfile("bot_brain.brn"):
    kernel.bootstrap(brainFile="bot_brain.brn")
else:
    kernel.bootstrap(learnFiles="basic.aiml", commands="LOAD AIML B")
    # Save the brain for next time
    kernel.saveBrain("bot_brain.brn")

# Simple loop to chat with the bot
print("Talk to the bot (type 'quit' to exit)")
while True:
    user_input = input("> ")
    if user_input.lower() == "quit":
        break
    response = kernel.respond(user_input)
    print(response)
