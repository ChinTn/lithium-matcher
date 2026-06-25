import csv
import random

# Let's generate 1 Million orders for our CSV
NUM_ORDERS = 3000000 

print(f"Generating {NUM_ORDERS} orders...")

with open('orders.csv', 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['id', 'side', 'price', 'quantity'])
    
    for i in range(NUM_ORDERS):
        order_id = i + 1
        side = 0 if random.random() < 0.5 else 1 # 0 = BUY, 1 = SELL
        price = random.randint(14500, 15500)
        quantity = random.randint(1, 100)
        writer.writerow([order_id, side, price, quantity])

print("Done! Saved to orders.csv")