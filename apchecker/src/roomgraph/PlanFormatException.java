package roomgraph;

/** Thrown when the plan file is missing, malformed, or internally inconsistent. */
public class PlanFormatException extends Exception {

    private static final long serialVersionUID = 1L;

    public PlanFormatException(String message) {
        super(message);
    }

    public PlanFormatException(String message, Throwable cause) {
        super(message, cause);
    }
}
