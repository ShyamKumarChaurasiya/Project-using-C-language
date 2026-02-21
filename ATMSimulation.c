#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h> // For exit()
#include <ctype.h>  // For isdigit()

// Global variables
long balanceCents = 1000000;
char currentPin[5] = "1234";

// Configuration limits
#define MAX_TRANSACTIONS 10
#define MAX_TRANSACTION_LIMIT 10000000.00 // $10,000,000.00 hard limit to prevent overflow

long transactionHistoryCents[MAX_TRANSACTIONS];
int transactionCount = 0;

// Function Prototypes
void checkBalance();
void depositMoney();
void withdrawMoney();
void displayMenu();
bool authenticate();
bool changePin();
void miniStatement();
void recordTransaction(long amountCents);
void getSafeInput(char* buffer, int size);

int main() {
    char inputBuffer[256];
    int choice;
    bool isRunning = true;

    // Initial Authentication Step
    if (!authenticate()) {
        printf("Too many failed attempts. Access Denied. Exiting ATM...\n");
        return 0;
    }

    while (isRunning) {
        displayMenu();

        getSafeInput(inputBuffer, sizeof(inputBuffer));

        if (strlen(inputBuffer) == 0) {
            continue;
        }

        if (sscanf(inputBuffer, "%d", &choice) != 1) {
            choice = -1;
        }

        switch (choice) {
            case 1:
                checkBalance();
                break;
            case 2:
                depositMoney();
                break;
            case 3:
                withdrawMoney();
                break;
            case 4:
                if (changePin()) {
                    printf("\n--- Security check: Please re-authenticate with your new PIN ---\n");
                    if (!authenticate()) {
                        printf("Too many failed attempts. Access Denied. Exiting ATM...\n");
                        isRunning = false;
                    } else {
                        printf(">>> Re-authentication successful! You may continue.\n");
                    }
                }
                break;
            case 5:
                miniStatement();
                break;
            case 6:
                printf("\nThank you for using the ATM. Goodbye!\n");
                isRunning = false;
                break;
            default:
                printf("\nInvalid selection. Please try again.\n");
        }
    }

    return 0;
}

// --- Function Definitions ---

void getSafeInput(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        if (strchr(buffer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }
        buffer[strcspn(buffer, "\r\n")] = '\0';
    } else {
        // FIX 1: Prevent "Stream Death" EOF Lockup
        // If the stream is broken (e.g., Ctrl+D / Ctrl+Z), shut down securely.
        printf("\n\n[System Error] Input stream disconnected. Securing terminal...\n");
        exit(1);
    }
}

bool authenticate() {
    char enteredPin[256];
    int maxAttempts = 3;

    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        printf("Enter your 4-digit PIN: ");
        getSafeInput(enteredPin, sizeof(enteredPin));

        if (strcmp(enteredPin, currentPin) == 0) {
            return true;
        } else {
            int remainingChances = maxAttempts - attempt;
            if (remainingChances > 0) {
                printf("Incorrect PIN. You have %d chance(s) left.\n\n", remainingChances);
            }
        }
    }
    return false;
}

void displayMenu() {
    printf("\n---------- ATM MENU ----------\n");
    printf("1. Check Balance\n");
    printf("2. Deposit\n");
    printf("3. Withdraw\n");
    printf("4. Change PIN\n");
    printf("5. Mini Statement\n");
    printf("6. Exit\n");
    printf("------------------------------\n");
    printf("Choose an option: ");
}

void checkBalance() {
    printf("\n>>> Your current balance is: $%.2f\n", balanceCents / 100.0);
}

void depositMoney() {
    char inputBuffer[256];
    double amount;

    printf("\nEnter amount to deposit: ");
    getSafeInput(inputBuffer, sizeof(inputBuffer));

    if (strlen(inputBuffer) == 0) {
        printf("Transaction cancelled.\n");
        return;
    }

    if (sscanf(inputBuffer, "%lf", &amount) == 1) {
        // FIX 2: Prevent "Infinite Money" NaN/Inf exploits
        if (!isfinite(amount)) {
            printf("Error: Invalid number format.\n");
            return;
        }

        // FIX 3: Prevent "Billionaire" Integer Overflow
        if (amount > MAX_TRANSACTION_LIMIT) {
            printf("Error: Deposit amount exceeds system limits.\n");
            return;
        }

        if (amount <= 0) {
            printf("Amount too small or invalid.\n");
            return;
        }

        long cents = (long)round(amount * 100.0);
        balanceCents += cents;
        recordTransaction(cents);
        printf("Successfully deposited $%.2f\n", cents / 100.0);

    } else {
        printf("Error: Invalid input.\n");
    }
}

void withdrawMoney() {
    char inputBuffer[256];
    double amount;

    printf("\nEnter amount to withdraw: ");
    getSafeInput(inputBuffer, sizeof(inputBuffer));

    if (strlen(inputBuffer) == 0) {
        printf("Transaction cancelled.\n");
        return;
    }

    if (sscanf(inputBuffer, "%lf", &amount) == 1) {
        // Guard against Inf/NaN and Overflow
        if (!isfinite(amount) || amount > MAX_TRANSACTION_LIMIT) {
            printf("Error: Invalid or excessive withdrawal amount.\n");
            return;
        }

        if (amount <= 0) {
            printf("Amount too small or invalid.\n");
            return;
        }

        long cents = (long)round(amount * 100.0);

        if (cents > balanceCents) {
            printf("Error: Insufficient funds!\n");
        } else {
            balanceCents -= cents;
            recordTransaction(-cents);
            printf("Successfully withdrew $%.2f\n", cents / 100.0);
        }
    } else {
        printf("Error: Invalid input.\n");
    }
}

bool changePin() {
    char oldPin[256], newPin[256], confirmPin[256];

    printf("\nEnter your current PIN: ");
    getSafeInput(oldPin, sizeof(oldPin));

    if (strcmp(oldPin, currentPin) == 0) {
        printf("Enter new 4-digit PIN: ");
        getSafeInput(newPin, sizeof(newPin));

        // Validation 1: Exactly 4 characters
        if (strlen(newPin) != 4) {
            printf("Error: PIN must be exactly 4 digits. Change aborted.\n");
            return false;
        }

        // FIX 4: Validation 2 - The "Alphabet" PIN Flaw (Must be numbers only)
        for (int i = 0; i < 4; i++) {
            if (!isdigit(newPin[i])) {
                printf("Error: PIN can only contain numbers (0-9). Change aborted.\n");
                return false;
            }
        }

        printf("Confirm new PIN: ");
        getSafeInput(confirmPin, sizeof(confirmPin));

        if (strcmp(newPin, confirmPin) == 0) {
            strcpy(currentPin, newPin);
            printf(">>> PIN successfully changed!\n");
            return true;
        } else {
            printf("Error: The new PINs do not match. Change aborted.\n");
            return false;
        }
    } else {
        printf("Error: Incorrect current PIN. Change aborted.\n");
        return false;
    }
}

void recordTransaction(long amountCents) {
    if (transactionCount < MAX_TRANSACTIONS) {
        transactionHistoryCents[transactionCount] = amountCents;
        transactionCount++;
    } else {
        for(int i = 1; i < MAX_TRANSACTIONS; i++) {
            transactionHistoryCents[i-1] = transactionHistoryCents[i];
        }
        transactionHistoryCents[MAX_TRANSACTIONS - 1] = amountCents;
    }
}

void miniStatement() {
    printf("\n--- Mini Statement (Recent Transactions) ---\n");
    if (transactionCount == 0) {
        printf("No transactions yet.\n");
    } else {
        for (int i = 0; i < transactionCount; i++) {
            if (transactionHistoryCents[i] > 0) {
                printf("Deposit:  +$%.2f\n", transactionHistoryCents[i] / 100.0);
            } else {
                printf("Withdraw: -$%.2f\n", (transactionHistoryCents[i] * -1) / 100.0);
            }
        }
    }
    printf("--------------------------------------------\n");
    printf("Available Balance: $%.2f\n", balanceCents / 100.0);
    printf("--------------------------------------------\n");
}
